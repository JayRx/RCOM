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

  setConnection(argv[1], status);

  al.fragmentSize = 4;

  if (al.status == TRANSMITTER) {
    sendData();
  } else if (al.status == RECEIVER) {
    readData();
  }

  llclose(al.fileDescriptor);

  return 0;
}

int getFileToRead() {
  printf("Name of the file to be transmitted: ");
  al.filename = (char *) malloc(64 * sizeof(char));
  //scanf("%s", al.filename);
  strcpy(al.filename, "pinguim.gif");
  if (al.filename == NULL) {
    printf("Error: Null Filename!\n");
    return -1;
  }

  al.fileDescriptorTBT = open(al.filename, O_RDONLY);
  if (al.fileDescriptorTBT < 0) {
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

  al.fileDescriptorTBT = open(al.filename, O_CREAT|O_WRONLY|O_APPEND, S_IWUSR|S_IRUSR);
  if (al.fileDescriptor < 0) {
    perror("Error when opening file");
    return -1;
  }

  return 0;
}

int sendData() {
  unsigned char* message;
  int package_message_size = 0, seq_number = 0;

  message = (unsigned char*) malloc(al.fragmentSize + 1);

  sendControlPackage(START_PACKAGE);

  while ((package_message_size = read(al.fileDescriptorTBT, message, al.fragmentSize)) > 0) {
    sendDataPackage(message, seq_number, package_message_size);
    seq_number++;
  }

  sendControlPackage(END_PACKAGE);

  if (close(al.fileDescriptorTBT) < 0) {
    printf("Error when closing file!\n");
    return -1;
  }

  free(al.filename);
  free(message);

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

  return 0;
}

int sendControlPackage(unsigned char byte) {
  struct stat file_info;
  int start_package_length;

  if (byte == START_PACKAGE) {
    if (getFileToRead() < 0) {
      printf("Error when opening the file!\n");
      return -1;
    }
  }

  if (fstat(al.fileDescriptorTBT, &file_info) < 0) {
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

  printf("Filename: %s\tSize: %ld\n", al.filename, al.fileSize);

  return 0;
}

int readData() {
  unsigned char* message;
  int counter = 0, length = 0;

  readControlPackage();

  printf("Filename: %s\tSize: %ld\n", al.filename, al.fileSize);

  while(counter < al.fileSize) {
    length = readDataPackage(&message);

    if (write(al.fileDescriptorTBT, message, length) <= 0) {
      perror("Couldn't write to file");
    }

    counter += length;

    printf("Counter: %d\tTotal: %ld\n", counter, al.fileSize);

    free(message);
  }

  readControlPackage();

  if (close(al.fileDescriptorTBT) < 0) {
    printf("Error when closing file!\n");
    return -1;
  }

  free(al.filename);
  free(al.fileData);

  return 0;
}

int readDataPackage(unsigned char** package_message) {
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
      strcpy(al.filename, (char *)&control_package[package_index + 1]);
      getFileToWrite();
    } else {
      printf("Couldn't recognise Control Packet!\n");
    }
  }

  free(control_package);

  return 0;
}

int setConnection(char* port, int status) {
  // Set ApplicationLayer struct
	al.status = status;

  // Set LinkLayer struct
  strcpy(linkLayer.port, port);
  linkLayer.baudRate = BAUDRATE;
  linkLayer.sequenceNumber = 0;
  linkLayer.timeout = TIMEOUT;
  linkLayer.numTransmissions = NUMTRANSMISSIONS;

  llopen(linkLayer.port, al.status);

  return 0;
}

int checkUsage(int argc, char** argv, int* status)  {
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
	} else if (strcmp("reader", argv[2])==0) {
		*status = RECEIVER;
	} else {
		printUsage();
		exit(1);
	}

  return 0;
}

void printUsage() {
	printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1 writer\n");
}
