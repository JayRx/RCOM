#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define TIMEOUT 1
#define NUMTRANSMISSIONS 3

struct applicationLayer {
  int fileDescriptor; // Descriptor of serial port
  int status; // TRANSMITTER | RECEIVER
};
