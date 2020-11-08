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

extern struct applicationLayer al;
extern struct linkLayer linkLayer;

int alarm_no=0;
enum states_UA current_state_UA = START_UA;
enum states_SET current_state_SET = START_SET;
enum states_I current_state_I = START_I;
enum states_RR_REJ current_state_RR_REJ = START_RR_REJ;
enum states_DISC current_state_DISC = START_DISC;
enum alarm_IDs current_alarm_ID = ALARM_SET;
struct termios oldtio,newtio;
volatile int STOP=FALSE;

int i = 0, package_message_size = 0;
unsigned char* package_message;
volatile int STOP_READING=FALSE;
unsigned char command_message[5];
extern unsigned char *messageIO;
extern unsigned char *stuffedFrame;
extern unsigned char *destuffedFrame;
extern unsigned char *dbcc;

int llopen(char port[20], int status) {
  int fd;

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

  // Set ApplicationLayer struct
  al.fileDescriptor = fd;

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

  return fd;
}

int llwrite(int fd, unsigned char *buffer, int length) {
  int res;

  package_message = buffer;
  package_message_size = length;

  res = write_I(fd, linkLayer.sequenceNumber, buffer, length);

  if (res <= 0)
    llwrite(fd, package_message, package_message_size);

  if (read_RR_REJ(fd))
    llwrite(fd, package_message, package_message_size);

  return 0;
}

int llread(int fd, unsigned char* buffer) {
  int res;

  res = read_I(fd, buffer);

  if(res > 0)
    write_RR(fd);
  else
    write_REJ(fd);

  return res;
}

int llclose(int fd) {
  if (al.status == TRANSMITTER) {
    current_alarm_ID = ALARM_DISC;
    write_DISC(fd, TRANSMITTER);
    read_DISC(fd);
    write_UA(fd, TRANSMITTER);
    sleep(1);
  } else if (al.status == RECEIVER) {
    read_DISC(fd);
    current_alarm_ID = ALARM_DISC;
    write_DISC(fd, RECEIVER);
    read_UA(fd);
  }

  if (tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  if (close(fd) == -1) {
    perror("close");
    return -1;
  }

  return 1;
}

void atende() {
  if(alarm_no == 3) {
    printf("Error!\n");
    exit(1);
  }

	alarm_no++;

  if (current_alarm_ID == ALARM_SET) {
    if (current_state_UA != STOP_UA) {
      write_SET(al.fileDescriptor);
    } else {
      alarm_no = 0;
    }
  } else if (current_alarm_ID == ALARM_DISC) {
    if (current_state_DISC != STOP_DISC && current_state_UA != STOP_UA) {
      write_DISC(al.fileDescriptor, al.status);
    } else {
      alarm_no = 0;
    }
  } else if (current_alarm_ID == ALARM_I) {
    if (current_state_RR_REJ != STOP_RR_REJ) {
      printf("Error! Rewriting...\n");
      write_I(al.fileDescriptor, linkLayer.sequenceNumber, package_message, package_message_size);
    } else {
      alarm_no = 0;
    }
  }
}

void write_SET(int fd) {
  int res;

  command_message[0]=FLAG_SET;
  command_message[1]=A_CA;
  command_message[2]=SET;
  command_message[3]=BCC_SET;
  command_message[4]=FLAG_SET;

  res = write(fd, command_message, 5);

  alarm(linkLayer.timeout);
}

void read_SET(int fd) {
  reset_state_machines();

  int res;
  unsigned char byte;

  while (STOP==FALSE) {       /* loop for input */
    res = read(fd,&byte,1);   /* returns after 1 char has been input */
    if (res == -1)
      break;

    if (res == 0)
      continue;

    current_state_SET = determineState_SET(byte, current_state_SET);

    if(current_state_SET == STOP_SET)
      STOP=TRUE;
  }
}

void write_UA(int fd, int status) {
  int res;

  if (status == TRANSMITTER) {
    command_message[0]=FLAG_UA;
    command_message[1]=A_AC;
    command_message[2]=UA;
    command_message[3]=BCC_UA_TRANSMITTER;
    command_message[4]=FLAG_UA;
  } else if (status == RECEIVER) {
    command_message[0]=FLAG_UA;
    command_message[1]=A_CA;
    command_message[2]=UA;
    command_message[3]=BCC_UA_RECEIVER;
    command_message[4]=FLAG_UA;
  } else {
    return;
  }

  res = write(fd, command_message, 5);
}

void read_UA(int fd) {
  reset_state_machines();

  int res;
  unsigned char byte;

  /* loop for input */
  while (STOP==FALSE) {       /* loop for input */
    res = read(fd,&byte,1);   /* returns after 1 char has been input */
    if (res == -1)
      break;

    if (res == 0)
      continue;

    current_state_UA = determineState_UA(byte, current_state_UA);

    if(current_state_UA == STOP_UA)
      STOP=TRUE;
  }
}

int write_I(int fd, int id, unsigned char *package_message, int length) {
  int res;
  unsigned int totalLength = 6 + length;
  unsigned char BCC2;

  current_alarm_ID = ALARM_I;
  reset_state_machines();

  messageIO[0]=FLAG_I;
  messageIO[1]=A_CA;
  if (id == 0) {
    messageIO[2]=I0;
    messageIO[3]=BCC1_I0;
  } else if (id == 1) {
    messageIO[2]=I1;
    messageIO[3]=BCC1_I1;
  }

  messageIO[4] = package_message[0];
  BCC2 = messageIO[4];
  for (int i = 1; i < length; i++){
    messageIO[i + 4] = package_message[i];
    BCC2 = BCC2 ^ messageIO[i + 4];
  }
  messageIO[4 + length]=BCC2;
  messageIO[4 + length + 1]=FLAG_SET;

  byteStuffing(messageIO, &totalLength);

  res = write(fd, stuffedFrame, totalLength);

  alarm(linkLayer.timeout);

  return res;
}

int read_I(int fd, unsigned char *package_message) {
  int res;
  unsigned char byte;
  unsigned int length = 0;

  reset_state_machines();

  /* loop for input */
  while (STOP==FALSE) {       /* loop for input */
    res = read(fd,&byte,1);   /* returns after 1 char has been input */

    if (res == -1)
      continue;

    if (res == 0)
      continue;

    current_state_I = determineState_I(byte, current_state_I);

    if (current_state_I == C_RCV_I) {
      if (byte >> 6 == linkLayer.sequenceNumber)
        res = -2;
    }

    if (current_state_I == STOP_I)
      STOP=TRUE;

    if (current_state_I == D_RCV_I) {
      if(STOP_READING==FALSE) {
        length++;
        dbcc[length - 1] = byte;
      }
    }
  }

  if (res == -1) {
    return -1;
  }

  byteDestuffing(dbcc, &length);

  if (!checkBCC(destuffedFrame, length)) {
    printf("Wrong BCC. Sending REJ!\n");
    res = -2;
  }

  length--;
  for (int i = 0; i < length; i++) {
    package_message[i] = destuffedFrame[i];
  }

  if (res == -2)
    return -2;

  return length;
}

void write_RR(int fd) {
  int res;

  command_message[0]=FLAG_RR;
  command_message[1]=A_CA;
  if (linkLayer.sequenceNumber == 0) {
    command_message[2]=RR0;
    command_message[3]=BCC_RR0;
  } else if (linkLayer.sequenceNumber == 1) {
    command_message[2]=RR1;
    command_message[3]=BCC_RR1;
  }
  command_message[4]=FLAG_RR;

  res = write(fd, command_message, 5);
}


int read_RR_REJ(int fd) {
  reset_state_machines();

  int res, aux = -1;
  unsigned char byte;

  /* loop for input */
  while (STOP==FALSE) {       /* loop for input */
    res = read(fd,&byte,1);   /* returns after 1 char has been input */
    if (res == -1)
      break;

    if (res == 0)
      continue;

    current_state_RR_REJ = determineState_RR_REJ(byte, current_state_RR_REJ);

    if(current_state_RR_REJ == STOP_RR_REJ)
      STOP=TRUE;

    if(current_state_RR_REJ == C_RCV_RR) {
      aux = 0;
    }

    if(current_state_RR_REJ == C_RCV_REJ) {
      aux = 1;
    }
  }

  return aux;
}

void write_REJ(int fd) {
  int res;

  command_message[0]=FLAG_REJ;
  command_message[1]=A_CA;
  if (linkLayer.sequenceNumber == 0) {
    command_message[2]=REJ0;
    command_message[3]=BCC_REJ0;
  } else if (linkLayer.sequenceNumber == 1) {
    command_message[2]=REJ1;
    command_message[3]=BCC_REJ1;
  }
  command_message[4]=FLAG_REJ;

  res = write(fd, command_message, 5);
}

void write_DISC(int fd, int status) {
  int res;

  if (status == TRANSMITTER) {
    command_message[0]=FLAG_DISC;
    command_message[1]=A_CA;
    command_message[2]=DISC;
    command_message[3]=BCC_DISC_TRANSMITTER;
    command_message[4]=FLAG_DISC;
  } else if (status == RECEIVER) {
    command_message[0]=FLAG_DISC;
    command_message[1]=A_AC;
    command_message[2]=DISC;
    command_message[3]=BCC_DISC_RECEIVER;
    command_message[4]=FLAG_DISC;
  } else {
    return;
  }

  res = write(fd, command_message, 5);

  alarm(linkLayer.timeout);
}

void read_DISC(int fd) {
  reset_state_machines();

  int res;
  unsigned char byte;

  /* loop for input */
  while (STOP==FALSE) {       /* loop for input */
    res = read(fd,&byte,1);   /* returns after 1 char has been input */
    if (res == -1)
      break;

    if (res == 0)
      continue;

    current_state_DISC = determineState_DISC(byte, current_state_DISC);

    if(current_state_DISC == STOP_DISC)
      STOP=TRUE;
  }
}

int checkBCC(unsigned char *data, int length) {
  int i;
  unsigned char BCC2 = data[0];

  // exluir BCC2 (ocupa 1 byte)
  for (i = 1; i < length - 1; i++) {
    BCC2 ^= data[i];
  }
  //último elemento -> BCC2
  if (BCC2 == data[length - 1]) {
    return TRUE;
  } else {
    printf("BCC2 doesn't check\nBCC2: %x, real BCC2: %x\n", data[length - 1],
           BCC2);
    return FALSE;
  }
}

int byteStuffing(unsigned char *frame, unsigned int *length) {
  unsigned int finalLength = *length;

  int i, j = 0;
  stuffedFrame[j++] = FLAG_I;

  // Do stuffing of all except flags
  for (i = 1; i < *length - 1; i++) {
    if (frame[i] == FLAG_I) {
      finalLength++;
      stuffedFrame[j] = ESCAPE;
      stuffedFrame[++j] = PATTERNFLAG;
      j++;
      continue;
    } else if (frame[i] == ESCAPE) {
      finalLength++;
      stuffedFrame[j] = ESCAPE;
      stuffedFrame[++j] = PATTERNESCAPE;
      j++;
      continue;
    } else {
      stuffedFrame[j++] = frame[i];
    }
  }

  stuffedFrame[j] = FLAG_I;

  *length = finalLength;

  return 0;
}

int byteDestuffing(unsigned char *data, unsigned int *length) {
  unsigned int finalLength = 0;

  int i;
  for (i = 0; i < *length; i++) {

    if (data[i] == ESCAPE) {
      if (data[i + 1] == PATTERNFLAG) {
        finalLength++;
        destuffedFrame[finalLength - 1] = FLAG_I;
        i++;
        continue;
      } else if (data[i + 1] == PATTERNESCAPE) {
        finalLength++;
        destuffedFrame[finalLength - 1] = ESCAPE;
        i++;
        continue;
      }
    }

    else {
      finalLength++;
      destuffedFrame[finalLength - 1] = data[i];
    }
  }

  *length = finalLength;
  return 0;
}

void reset_state_machines() {
  STOP = FALSE;
  current_state_SET = START_SET;
  current_state_UA = START_UA;
  current_state_I = START_I;
  current_state_RR_REJ = START_RR_REJ;
  current_state_DISC = START_DISC;
}
