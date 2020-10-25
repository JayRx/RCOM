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

    if ( (argc < 2) ||
  	     ((strcmp("/dev/ttyS10", argv[1])!=0) &&
  	      (strcmp("/dev/ttyS11", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

    // Set LinkLayer struct
    strcpy(linkLayer.port, argv[1]);
    linkLayer.baudRate = BAUDRATE;
    linkLayer.sequenceNumber = 0;
    linkLayer.timeout = TIMEOUT;
    linkLayer.numTransmissions = NUMTRANSMISSIONS;

    llopen(linkLayer.port, TRANSMITTER);

    llclose(applicationLayer.fileDescriptor);

    return 0;
}
