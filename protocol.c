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

int conta=0;
enum states_UA current_state_UA = START_UA;
struct termios oldtio,newtio;
char buf[255];
volatile int STOP=FALSE;

int i = 0;
char message[255];

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
  applicationLayer.status = TRANSMITTER;

  /*
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar
    o indicado no guião
  */

  if (status == TRANSMITTER) {
    (void) signal(SIGALRM, atende);

    write_SET(fd);

    read_UA(fd);
  }

  return -1;
}

int llwrite(int fd, char *buffer, int length) {
  /* TODO */
  return -1;
}

int llread(int fd, char *buffer) {
  /* TODO */
  return -1;
}

int llclose(int fd) {

  if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  close(fd);

  return -1;
}

void atende() {

	printf("ALARME #%d\n", conta);

  if(conta == 3) {
    exit(1);
  }

	conta++;

  if(current_state_UA != STOP_UA)
    write_SET(applicationLayer.fileDescriptor);

}

void write_SET(int fd) {
  printf("----- Writing SET -----\n");
  int res;

  buf[0]=FLAG_WR;
  buf[1]=A_WR;
  buf[2]=SET;
  buf[3]=BCC_WR;
  buf[4]=FLAG_WR;

  res = write(fd, buf, 5);
  printf("%d bytes written\n\n", res);

  alarm(3);
}

void read_UA(int fd) {
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
