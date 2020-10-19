#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define FLAG_WR 0x7e
#define A_WR 0x03
#define SET 0x03
#define BCC_WR (A_WR^SET)

#define FLAG_RD 0x7e
#define A_RD 0x03
#define UA 0x07
#define BCC_RD (A_RD^UA)

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

enum states_SET determineState_SET(char byte, enum states_SET s);

enum states_UA determineState_UA(char byte, enum states_UA s);

void printState_SET(enum states_SET s);

void printState_UA(enum states_UA s);
