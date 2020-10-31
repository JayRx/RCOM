#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "state_machine.h"
#include "application.h"

#include "protocol.h"

extern struct applicationLayer applicationLayer;
extern struct linkLayer linkLayer;

int alarm_no=0;
enum states_UA current_state_UA = START_UA;
enum states_SET current_state_SET = START_SET;
enum states_I current_state_I = START_I;
enum states_RR_REJ current_state_RR_REJ = START_RR_REJ;
enum states_DISC current_state_DISC = START_DISC;
enum alarm_IDs current_alarm_ID = ALARM_SET;
struct termios oldtio,newtio;
unsigned char buf[255];
volatile int STOP=FALSE;

int i = 0;
char message[255];
char package_message[4];

int llopen(char port[20], int status) {
  int fd;
  int c;
  int sum = 0, speed = 0;

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

  fd = open(linkLayer.port, O_RDWR | O_NOCTTY );
  if (fd <0) {perror(linkLayer.port); exit(-1); }

  if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = linkLayer.baudRate | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
  newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

  /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
  */

  tcflush(fd, TCIOFLUSH);

  if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  printf("New termios structure set\n\n");

  // Set ApplicationLayer struct
  applicationLayer.fileDescriptor = fd;

  /*
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar
    o indicado no guião
  */

  (void) signal(SIGALRM, atende);

  if (status == TRANSMITTER) {
    current_alarm_ID = ALARM_SET;
    write_SET(fd);
    read_UA(fd);
  } else if (status == RECEIVER) {
  	read_SET(fd);
    write_UA(fd, RECEIVER);
  }

  return -1;
}

int llwrite(int fd, char *buffer, int length) {
  int j, h;

  current_alarm_ID = ALARM_I;

  if (applicationLayer.status == TRANSMITTER) {
    for (h = 0; h < length / 4; h++) {
      for (j = 0; j < 4; j++) {
        package_message[j] = buffer[h * 4 + j];
      }
      reset_state_machines();
      write_I(fd, linkLayer.sequenceNumber, package_message);
      if(read_RR_REJ(fd))
        write_I(fd, linkLayer.sequenceNumber, package_message);
      linkLayer.sequenceNumber = !linkLayer.sequenceNumber;
    }
  }

  return -1;
}

int llread(int fd, char *buffer) {
  char *package_message;

  if (applicationLayer.status == RECEIVER) {
    do {
      package_message = read_I(fd);
      linkLayer.sequenceNumber = !linkLayer.sequenceNumber;
      if(package_message != NULL)
        write_RR(fd);
    } while(package_message != NULL);
  }

  return -1;
}

int llclose(int fd) {

  if (applicationLayer.status == TRANSMITTER) {
    current_alarm_ID = ALARM_DISC;
    write_DISC(fd, TRANSMITTER);
    read_DISC(fd);
    write_UA(fd, TRANSMITTER);
  } else if (applicationLayer.status == RECEIVER) {
    read_DISC(fd);
    current_alarm_ID = ALARM_DISC;
    write_DISC(fd, RECEIVER);
    read_UA(fd);
  }

  if (tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  close(fd);

  return -1;
}

void atende() {

	printf("ALARME #%d\n", alarm_no);

  if(alarm_no == 3) {
    exit(1);
  }

	alarm_no++;

  if (current_alarm_ID == ALARM_SET) {
    if (current_state_UA != STOP_UA) {
      write_SET(applicationLayer.fileDescriptor);
    } else {
      alarm_no = 0;
    }
  } else if (current_alarm_ID == ALARM_DISC) {
    if (current_state_DISC != STOP_DISC && current_state_UA != STOP_UA) {
      write_DISC(applicationLayer.fileDescriptor, applicationLayer.status);
    } else {
      alarm_no = 0;
    }
  } else if (current_alarm_ID == ALARM_I) {
    if (current_state_RR_REJ != STOP_RR_REJ /*current_state_RR != STOP_RR && current_state_REJ != STOP_REJ*/) {
      write_I(applicationLayer.fileDescriptor, linkLayer.sequenceNumber, package_message);
    } else {
      alarm_no = 0;
    }
  }
}

void write_SET(int fd) {
  printf("----- Writing SET -----\n");
  int res;

  buf[0]=FLAG_SET;
  buf[1]=A_CA;
  buf[2]=SET;
  buf[3]=BCC_SET;
  buf[4]=FLAG_SET;

  res = write(fd, buf, 5);
  printf("%d bytes written\n\n", res);

  alarm(3);
}

void read_SET(int fd) {
  reset_state_machines();

  printf("----- Reading SET -----\n");
  int res;

  while (STOP==FALSE) {       /* loop for input */
    res = read(fd,buf,1);   /* returns after 1 char has been input */
    if (res == -1)
      break;

    printf("nº bytes lido: %d - ", res);
    printf("content: 0x%x\n", buf[0]);

    current_state_SET = determineState_SET(buf[0], current_state_SET);

    if(current_state_SET == STOP_SET)
      STOP=TRUE;

    printState_SET(current_state_SET);
    message[i] = buf[0];
    i++;
  }
  printf("\n");
}

void write_UA(int fd, int status) {
  printf("----- Writing UA -----\n");
  int res;

  if (status == TRANSMITTER) {
    buf[0]=FLAG_UA;
    buf[1]=A_AC;
    buf[2]=UA;
    buf[3]=BCC_UA_TRANSMITTER;
    buf[4]=FLAG_UA;
  } else if (status == RECEIVER) {
    buf[0]=FLAG_UA;
    buf[1]=A_CA;
    buf[2]=UA;
    buf[3]=BCC_UA_RECEIVER;
    buf[4]=FLAG_UA;
  } else {
    return;
  }

  res = write(fd, buf, 5);

  printf("%d bytes written\n\n", res);
}

void read_UA(int fd) {
  reset_state_machines();

  printf("----- Reading UA -----\n");
  int res;

  /* loop for input */
  while (STOP==FALSE) {       /* loop for input */
    res = read(fd,buf,1);   /* returns after 1 char has been input */
    if (res == -1)
      break;

    printf("nº bytes lido: %d - ", res);
    printf("content: 0x%x\n", buf[0]);

    current_state_UA = determineState_UA(buf[0], current_state_UA);

    if(current_state_UA == STOP_UA)
      STOP=TRUE;

    printState_UA(current_state_UA);
    message[i] = buf[0];
    i++;
  }
  printf("\n");
}

void write_I(int fd, int id, char *package_message) {
  printf("----- Writing I %d -----\n", id);
  int res;

  buf[0]=FLAG_I;
  buf[1]=A_CA;
  if (id == 0) {
    buf[2]=I0;
    buf[3]=BCC1_I0;
    buf[8]=BCC1_I0;
  } else if (id == 1) {
    buf[2]=I1;
    buf[3]=BCC1_I1;
    buf[8]=BCC1_I1;
  }
  for (int i = 0; i < 3; i++){
    buf[i + 4] = package_message[i];
  }
  buf[9]=FLAG_SET;

  for (int i = 0; i < 10; i++)
    printf("buf[%d] : 0x%x\n", i, buf[i]);

  res = write(fd, buf, 10);
  printf("%d bytes written\n\n", res);

  alarm(3);
}

char* read_I(int fd) {
  char* package_message;
  int j=0, res;

  package_message = (char*) calloc(5, sizeof(char));

  reset_state_machines();

  printf("----- Reading I %d-----\n", linkLayer.sequenceNumber);

  /* loop for input */
  while (STOP==FALSE) {       /* loop for input */
    res = read(fd,buf,1);   /* returns after 1 char has been input */
    if (res == -1)
      break;

    printf("nº bytes lido: %d - ", res);
    printf("content: 0x%x\n", buf[0]);

    current_state_I = determineState_I(buf[0], current_state_I);

    if (current_state_I == STOP_I)
      STOP=TRUE;
    else if (current_state_I == OTHER_RCV_I)
      return NULL;

    printState_I(current_state_I);
    package_message[j] = buf[0];
    j++;
  }
  printf("\n");

  return package_message;
}

void write_RR(int fd) {
  printf("----- Writing RR -----\n");
  int res;

  buf[0]=FLAG_RR;
  buf[1]=A_CA;
  if (linkLayer.sequenceNumber == 0) {
    buf[2]=RR0;
    buf[3]=BCC_RR0;
  } else if (linkLayer.sequenceNumber == 1) {
    buf[2]=RR1;
    buf[3]=BCC_RR1;
  }
  buf[4]=FLAG_RR;

  res = write(fd, buf, 5);

  printf("%d bytes written\n\n", res);
}


int read_RR_REJ(int fd) {
  reset_state_machines();

  printf("----- Reading RR or REJ -----\n");
  int res, aux = -1;

  /* loop for input */
  while (STOP==FALSE) {       /* loop for input */
    res = read(fd,buf,1);   /* returns after 1 char has been input */
    if (res == -1)
      break;

    printf("nº bytes lido: %d - ", res);
    printf("content: 0x%x\n", buf[0]);

    current_state_RR_REJ = determineState_RR_REJ(buf[0], current_state_RR_REJ);

    if(current_state_RR_REJ == STOP_RR_REJ)
      STOP=TRUE;

    if(current_state_RR_REJ == C_RCV_RR)
      aux = 0;

    if(current_state_RR_REJ == C_RCV_REJ)
      aux = 1;

    printState_RR_REJ(current_state_RR_REJ);
    message[i] = buf[0];
    i++;
  }
  printf("\n");

  return aux;
}

void write_REJ(int fd) {
  printf("----- Writing REJ -----\n");
  int res;

  buf[0]=FLAG_REJ;
  buf[1]=A_CA;
  if (linkLayer.sequenceNumber == 0) {
    buf[2]=REJ0;
    buf[3]=BCC_REJ0;
  } else if (linkLayer.sequenceNumber == 1) {
    buf[2]=REJ1;
    buf[3]=BCC_REJ1;
  }
  buf[4]=FLAG_REJ;

  res = write(fd, buf, 5);

  printf("%d bytes written\n\n", res);
}

void write_DISC(int fd, int status) {
  printf("----- Writing DISC -----\n");
  int res;

  if (status == TRANSMITTER) {
    buf[0]=FLAG_DISC;
    buf[1]=A_CA;
    buf[2]=DISC;
    buf[3]=BCC_DISC_TRANSMITTER;
    buf[4]=FLAG_DISC;
  } else if (status == RECEIVER) {
    buf[0]=FLAG_DISC;
    buf[1]=A_AC;
    buf[2]=DISC;
    buf[3]=BCC_DISC_RECEIVER;
    buf[4]=FLAG_DISC;
  } else {
    return;
  }

  res = write(fd, buf, 5);

  printf("%d bytes written\n\n", res);

  alarm(3);
}

void read_DISC(int fd) {
  reset_state_machines();

  printf("----- Reading DISC -----\n");
  int res;

  /* loop for input */
  while (STOP==FALSE) {       /* loop for input */
    res = read(fd,buf,1);   /* returns after 1 char has been input */
    if (res == -1)
      break;

    printf("nº bytes lido: %d - ", res);
    printf("content: 0x%x\n", buf[0]);

    current_state_DISC = determineState_DISC(buf[0], current_state_DISC);

    if(current_state_DISC == STOP_DISC)
      STOP=TRUE;

    printState_DISC(current_state_DISC);
    message[i] = buf[0];
    i++;
  }
  printf("\n");
}

void reset_state_machines() {
  STOP = FALSE;
  current_state_SET = START_SET;
  current_state_UA = START_UA;
  current_state_I = START_I;
  current_state_RR_REJ = START_RR_REJ;
  current_state_DISC = START_DISC;
}
