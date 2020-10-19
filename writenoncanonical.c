/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>

#include "state_machine.h"


#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

int conta=0;
char message[255];
int i = 0;

int fd, res;
char buf[255];

enum states_UA current_state_UA = START_UA;

void write_SET();

void atende() {

	printf("ALARME #%d\n", conta);

  if(conta == 3) {
    exit(1);
  }

	conta++;

  if(current_state_UA != STOP_UA)
    write_SET();

}

void write_SET() {
  printf("----- Writing SET -----\n");

  buf[0]=FLAG_WR;
  buf[1]=A_WR;
  buf[2]=SET;
  buf[3]=BCC_WR;
  buf[4]=FLAG_WR;

  res = write(fd, buf, 5);
  printf("%d bytes written\n\n", res);

  alarm(3);
}

void read_UA() {
  printf("----- Reading UA -----\n");

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

int main(int argc, char** argv) {
    int c;
    struct termios oldtio,newtio;
    int sum = 0, speed = 0;

    if ( (argc < 2) ||
  	     ((strcmp("/dev/ttyS10", argv[1])!=0) &&
  	      (strcmp("/dev/ttyS11", argv[1])!=0) )) {
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
    leitura do(s) próximo(s) caracter(es)
  */

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n\n");

    (void) signal(SIGALRM, atende);

    write_SET();

    read_UA();

    /*
      O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar
      o indicado no guião
    */

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);
    return 0;
}
