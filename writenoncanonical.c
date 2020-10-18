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


#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x7e
#define A 0x03
#define SET 0x03
#define BCC 0

#define START 0
#define FLAGRCV 1
#define ARCV 2
#define CRCV 3
#define BCCOK 4
#define STOP_STATE 5


volatile int STOP=FALSE;

typedef struct machine_state
{ 
    bool value;
    char name[10];
}state;

state states[6];

state determinestate(char byte, int state)
{
  if(state==BCCOK && byte == FLAG )
    return states[STOP_STATE];
  if(byte == FLAG ) 
    return states[FLAGRCV];
  if(byte == A && state==FLAGRCV)
    return states[ARCV];
  if(byte == SET && state==ARCV)
  return states[CRCV];
  if(state==CRCV && byte==A^SET)
      return states[BCCOK];
  return states[START];
}


int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;

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

    printf("New termios structure set\n");

    printf("Mensagem: ");
    //fgets(buf, 256, stdin);
    buf[0]=FLAG;
    buf[1]=A;
    buf[2]=SET;
    buf[3]=A^SET;
    buf[4]=FLAG;
    
  states[0].value=true;
  strcpy(states[0].name,"START");

  states[1].value=false;
  strcpy(states[1].name,"FLAGRCV");

  states[2].value=false;
  strcpy(states[2].name,"ARCV");

  states[3].value=false;
  strcpy(states[3].name,"CRCV");

  states[4].value=false;
  strcpy(states[4].name,"BCCOK");

  states[5].value=false;
  strcpy(states[5].name,"STOP");

  int current_state = START;

    res = write(fd, buf, 5);
    printf("%d bytes written\n", res);

     /* loop for input */
      while (STOP==FALSE) {       /* loop for input */
        res = read(fd,buf,1);   /* returns after 1 char has been input */
        if (res == -1)
          break;
        
        printf("nº bytes lido: %d - ", res);
        printf("content: %c\n", buf[0]);
        if(strcmp(determinestate(buf[0], current_state).name, "STOP")==0)
        STOP==TRUE;
      }
    

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
