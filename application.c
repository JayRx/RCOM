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

char buf[255];

struct applicationLayer applicationLayer;
struct linkLayer linkLayer;

int main(int argc, char** argv) {

  int status;

	checkUsage(argc, argv, &status);
	// Set ApplicationLayer struct
	applicationLayer.status = status;

  // Set LinkLayer struct
  strcpy(linkLayer.port, argv[1]);
  linkLayer.baudRate = BAUDRATE;
  linkLayer.sequenceNumber = 0;
  linkLayer.timeout = TIMEOUT;
  linkLayer.numTransmissions = NUMTRANSMISSIONS;

  llopen(linkLayer.port, applicationLayer.status);

  llclose(applicationLayer.fileDescriptor);

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
