#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define FLAG_SET 0x7e
#define A_SET 0x03
#define SET 0x03
#define BCC_SET (A_SET^SET)

#define FLAG_UA 0x7e
#define A_UA 0x03
#define UA 0x07
#define BCC_UA (A_UA^UA)

#define FLAG_DISC 0x7e
#define A_DISC 0x03
#define DISC 0x07
#define BCC_DISC (A_DISC^DISC)

enum states_SET {
  START_SET,
  FLAG_RCV_SET,
  A_RCV_SET,
  C_RCV_SET,
  BCC_OK_SET,
  STOP_SET,
  OTHER_RCV_SET
} state_SET;

enum states_UA {
  START_UA,
  FLAG_RCV_UA,
  A_RCV_UA,
  C_RCV_UA,
  BCC_OK_UA,
  STOP_UA,
  OTHER_RCV_UA
} state_UA;

enum states_DISC {
  START_DISC,
  FLAG_RCV_DISC,
  A_RCV_DISC,
  C_RCV_DISC,
  BCC_OK_DISC,
  STOP_DISC,
  OTHER_RCV_DISC
} state_DISC;

enum states_SET determineState_SET(char byte, enum states_SET s);

enum states_UA determineState_UA(char byte, enum states_UA s);

enum states_DISC determineState_DISC(char byte, enum states_DISC s);

void printState_SET(enum states_SET s);

void printState_UA(enum states_UA s);

void printState_DISC(enum states_DISC s);
