#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define TIMEOUT 10
#define NUMTRANSMISSIONS 3

#define DATA_PACKAGE 1
#define START_PACKAGE 2
#define END_PACKAGE 3

#define T_FILESIZE 0x00
#define T_FILENAME 0x01

struct applicationLayer {
  int fileDescriptor; // Descriptor of serial port
  int status; // TRANSMITTER | RECEIVER
  char* filename; // Name of the file to be transmitted
  FILE* fileDescriptorTBT; // Descriptor of the file to be transmitted
  off_t fileSize; // Size of the file to be transmitted
  unsigned int fragmentSize; // Max size of each fragment
};

int setConnection(char* port, int status);

int setDisconnection(char* port, int status);

int sendData();

int sendDataPackage(unsigned char *package_message, unsigned int seq_number, unsigned int length);

int sendControlPackage(unsigned char byte);

int readData();

int readDataPackage(unsigned char* package_message, int seq_number);

int readControlPackage();

void printUsage();

int checkUsage(int argc, char** argv, int* status);

void printProgressBar(int current, int total);

int intToBaudrate(int baudrate);
