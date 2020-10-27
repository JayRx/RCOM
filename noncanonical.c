/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "state_machine.h"

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

int fd, res;
char buf[255];
char message[255];
int i = 0;

enum states_SET current_state_SET = START_SET;

void write_UA() {
  printf("----- Writing UA -----\n");

  buf[0]=FLAG_RD;
  buf[1]=A_RD;
  buf[2]=UA;
  buf[3]=BCC_RD;
  buf[4]=FLAG_RD;

  res = write(fd, buf, 5);

  printf("%d bytes written\n\n", res);
}

void read_SET() {
  printf("----- Reading SET -----\n");

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

int main(int argc, char** argv) {
    int c;
    struct termios oldtio,newtio;

    if ( (argc < 2) ||
  	     ((strcmp("/dev/ttyS10", argv[1])!=0) &&
  	      (strcmp("/dev/ttyS11", argv[1])!=0) &&
		  (strcmp("/dev/ttyS0", argv[1])!=0))) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

  /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) pr�ximo(s) caracter(es)
  */

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n\n");

    read_SET();

    write_UA();

  /*
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no gui�o
  */

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
