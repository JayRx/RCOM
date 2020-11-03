#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "state_machine.h"
#include "protocol.h"

#include "application.h"

struct applicationLayer al;
struct linkLayer linkLayer;

int main(int argc, char** argv) {

  int status;

	checkUsage(argc, argv, &status);

  if (setConnection(argv[1], status) != 0) {
    printf("Error when connecting!\n");
    return -1;
  }

  if (al.status == TRANSMITTER) {
    sendData();
  } else if (al.status == RECEIVER) {
    readData();
  }

  setDisconnection(argv[1], status);

  return 0;
}

int getFileToRead() {
  if (al.filename == NULL) {
    printf("Error: Null Filename!\n");
    return -1;
  }

  al.fileDescriptorTBT = fopen((char*)al.filename, "rb");
  if (al.fileDescriptorTBT == NULL) {
    perror("Error when opening file");
    return -1;
  }

  return 0;
}

int getFileToWrite() {
  if (al.filename == NULL) {
    printf("Error: Null Filename!\n");
    return -1;
  }

  //al.fileDescriptorTBT = fopen((char*)"pinguim2.gif", "wb+");
  printf("Filename: %s\n", al.filename);
  al.fileDescriptorTBT = fopen((char*) al.filename, "wb+");
  if (al.fileDescriptorTBT == NULL) {
    perror("Error when opening file");
    return -1;
  }

  return 0;
}

int sendData() {
  unsigned char* message;
  int package_message_size = 0, seq_number = 0;
  int sent_size = 0;

  message = (unsigned char*) malloc(al.fragmentSize + 1);

  printf("\nSending Data...\n");

  if(sendControlPackage(START_PACKAGE) == -1) {
    printf("\nSending Data: Error\n");
    return -1;
  }

  while ((package_message_size = fread(message, sizeof(unsigned char), al.fragmentSize, al.fileDescriptorTBT)) > 0) {
    sendDataPackage(message, seq_number, package_message_size);
    seq_number++;
    sent_size += package_message_size;
    printProgressBar(sent_size, al.fileSize);
  }

  sendControlPackage(END_PACKAGE);

  if (fclose(al.fileDescriptorTBT) != 0) {
    printf("Error when closing file!\n");
    return -1;
  }

  free(al.filename);
  free(message);

  printf("\n");

  return 0;
}

int sendDataPackage(unsigned char *package_message, unsigned int seq_number, unsigned int length) {
  unsigned char data_package[255];
  int data_package_size = length + 4;

  data_package[0] = DATA_PACKAGE;
  data_package[1] = seq_number;
  data_package[2] = length / 256;
  data_package[3] = length % 256;
  for (int i = 0; i < length; i++) {
    data_package[i + 4] = package_message[i];
  }

  llwrite(al.fileDescriptor, data_package, data_package_size);

  linkLayer.sequenceNumber = !linkLayer.sequenceNumber;

  return 0;
}

int sendControlPackage(unsigned char byte) {
  struct stat file_info;
  int start_package_length;

  if (stat((char*)al.filename, &file_info) < 0) {
    perror("Couldn't get information about the target file");
    return -1;
  }

  off_t file_size = file_info.st_size; // Size of the file
  al.fileSize = file_size;
  unsigned int l1 = sizeof(file_size);
  unsigned int l2 = strlen(al.filename) + 1;

  start_package_length = l1 + l2 + 5;

  unsigned char start_package[start_package_length];

  start_package[0] = byte;
  start_package[1] = T_FILESIZE;
  start_package[2] = l1;
  *((off_t *)(start_package + 3)) = file_size;
  start_package[3 + l1] = T_FILENAME;
  start_package[3 + l1 + 1] = l2;
  strcat((char *) start_package + 5 + l1, al.filename);

  llwrite(al.fileDescriptor, start_package, start_package_length);

  if (byte == START_PACKAGE)
    printf("\nFilename: %s\tSize: %ld\n", al.filename, al.fileSize);

  return 0;
}

int readData() {
  printf("\nReading Data...\n");
  unsigned char* message;
  int counter = 0, length = 0;
  unsigned int seq_number = 0;

  readControlPackage();

  while(counter < al.fileSize) {
    length = readDataPackage(&message, seq_number);

    if (fwrite(message, sizeof(unsigned char), length, al.fileDescriptorTBT) <= 0) {
      perror("Couldn't write to file");
    } else {
      counter += length;
      seq_number++;
    }

    printProgressBar(counter, al.fileSize);

    free(message);
  }

  readControlPackage();

  if (fclose(al.fileDescriptorTBT) != 0) {
    printf("Error when closing file!\n");
    return -1;
  }

  free(al.filename);
  free(al.fileData);

  printf("\n");

  return 0;
}

int readDataPackage(unsigned char** package_message, int seq_number) {
  unsigned char* data_package;
  unsigned char C;
  int N, L1, L2, K;

  llread(al.fileDescriptor, &data_package);

  C = data_package[0];
  if (C != DATA_PACKAGE) {
    printf("Error: Not a Data Package!\n");
    return -1;
  }

  N = data_package[1];

  L2 = data_package[2];
  L1 = data_package[3];
  K = 256 * L2 + L1;

  *package_message = (unsigned char *) malloc(K);
  memcpy((*package_message), (data_package + 4), K);

  free(data_package);

  if (seq_number % 256 != N)
    return -1;

  linkLayer.sequenceNumber = !linkLayer.sequenceNumber;

  return K;
}

int readControlPackage() {
  unsigned char* control_package;

  llread(al.fileDescriptor, &control_package);
  int package_index = 0;
  unsigned int data_size;

  if (control_package[0] == END_PACKAGE) {
    free(control_package);
    return 0;
  }

  package_index++;

  unsigned char package_type;
  for (int i = 0; i < 2; i++) {
    package_type = control_package[package_index++];

    if (package_type == T_FILESIZE) {
      data_size = (unsigned int) control_package[package_index++];
      al.fileSize = *((off_t *)(control_package + package_index));
      al.fileData = (unsigned char*) malloc(al.fileSize * sizeof(char));
      package_index += data_size;
    } else if (package_type == T_FILENAME) {
      data_size = (unsigned int) control_package[package_index++];
      al.filename = (char*) malloc(data_size * sizeof(char) + 1);
      strcpy(al.filename, (char *)&control_package[package_index]);
      getFileToWrite();
    } else {
      printf("Couldn't recognise Control Packet!\n");
    }
  }

  free(control_package);

  return 0;
}

int setConnection(char* port, int status) {
  printf("\nConnecting: ");

  // Set ApplicationLayer struct
	al.status = status;

  // Set LinkLayer struct
  strcpy(linkLayer.port, port);
  if (status == TRANSMITTER)
   linkLayer.sequenceNumber = 0;
  else
    linkLayer.sequenceNumber = 1;
  linkLayer.timeout = TIMEOUT;
  linkLayer.numTransmissions = NUMTRANSMISSIONS;

  if (status == TRANSMITTER)
    if (getFileToRead() < 0) {
      printf("Error when opening the file!\n");
      return -1;
    }

  if(llopen(linkLayer.port, al.status) >= 0) {
    printf("Success!\n");
    return 0;
  } else {
    printf("Error!\n");
    return -1;
  }
}

int setDisconnection(char* port, int status) {
  printf("\nDisconnecting: ");

  if(llclose(al.fileDescriptor) == 1) {
    printf("Success!\n\n");
    return 0;
  } else {
    printf("Error!\n\n");
    return -1;
  }
}

int checkUsage(int argc, char** argv, int* status)  {
  printf("Checking Usage!\n");

	if (argc < 3) {
		printUsage();
		exit(1);
	}

	if ((strcmp("/dev/ttyS10", argv[1])!=0) &&
  	    (strcmp("/dev/ttyS11", argv[1])!=0) &&
		(strcmp("/dev/ttyS0", argv[1]) != 0)) {
	  	printUsage();
      	exit(1);
    }


	if (strcmp("writer", argv[2])==0) {
		*status = TRANSMITTER;
    if (argc != 6) {
  		printUsage();
  		exit(1);
  	}
    if (argv[3] == NULL) {
      printUsage();
      exit(1);
    }
    al.filename = (char *) malloc(64 * sizeof(char));
    strcpy(al.filename, argv[3]);
    al.fragmentSize = atoi(argv[4]);
    linkLayer.baudRate = intToBaudrate(atoi(argv[5]+1));
	} else if (strcmp("reader", argv[2])==0) {
		*status = RECEIVER;
    if (argc != 5) {
  		printUsage();
  		exit(1);
  	}
    al.fragmentSize = atoi(argv[3]);
    linkLayer.baudRate = intToBaudrate(atoi(argv[4]+1));
	} else {
		printUsage();
		exit(1);
	}

  return 0;
}

void printUsage() {
	printf("Usage:\trcom SerialPort writer File FragmentSize BaudRate\n\trcom SerialPort reader FragmentSize BaudRate\n\tEx: rcom ttyS0 writer pinguim.gif 16 B38400\n");
}

void printProgressBar(int current, int total) {
	float percentage = 100.0 * (float) current / (float) total;

	printf("\r[");

	int i, len = 30;
	int pos = percentage * len / 100.0;

	for (i = 0; i < len; i++)
		i <= pos ? printf("■") : printf(" ");

	printf("]  %6.2f%%", percentage);

	fflush(stdout);
}

int intToBaudrate(int baudrate) {
	switch (baudrate) {
	case 0:
		return B0;
	case 50:
		return B50;
	case 75:
		return B75;
	case 110:
		return B110;
	case 134:
		return B134;
	case 150:
		return B150;
	case 200:
		return B200;
	case 300:
		return B300;
	case 600:
		return B600;
	case 1200:
		return B1200;
	case 1800:
		return B1800;
	case 2400:
		return B2400;
	case 4800:
		return B4800;
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	case 460800:
		return B460800;
	default:
		return -1;
	}
}
