#define MAX_SIZE 256
#define TRANSMITTER 0
#define RECEIVER 1

struct linkLayer {
  char port[20]; // Dispositivo /dev/ttySx, x = 0, 1
  int baudRate; // Velocidade de transmissão
  unsigned int sequenceNumber; // Número de sequência da trama: 0, 1
  unsigned int timeout; // Valor do temporizador: 1s
  unsigned int numTransmissions; // Número de tentativas em caso de falha
  char frame[MAX_SIZE]; // Trama
};

enum alarm_IDs {
  ALARM_SET,
  ALARM_DISC,
  ALARM_I
} alarm_ID;

int llopen(char port[20], int status); // Return id of data connection if success or a negative number if error

int llwrite(int fd, char *buffer, int length); // Return array size (number of chars written) if success or a negative number if error

int llread(int fd, char *buffer); // Return array size (number of chars read) if success or a negative number if error

int llclose(int fd); // Return a positive number if success or a negative number if error

void write_SET(int fd);

void read_SET(int fd);

void write_UA(int fd, int status);

void read_UA(int fd);

void write_I(int fd, int id, char package_message[5]);

char* read_I(int fd);

void write_RR(int fd);

int read_RR_REJ(int fd);

//void read_RR(int fd);

void write_REJ(int fd);

//void read_REJ(int fd);

void write_DISC(int fd, int status);

void read_DISC(int fd);

void atende();

void reset_state_machines();
