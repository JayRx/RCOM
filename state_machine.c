#include "state_machine.h"

enum states_SET determineState_SET(char byte, enum states_SET s) {
  if (s == START_SET && byte == FLAG_WR)
    return FLAG_RCV_SET;

  if (s == FLAG_RCV_SET) {
    if (byte == A_WR)
      return A_RCV_SET;
    else if (byte == FLAG_WR)
      return FLAG_RCV_SET;
  }

  if (s == A_RCV_SET) {
    if (byte == SET)
      return C_RCV_SET;
    else if (byte == FLAG_WR)
      return FLAG_RCV_SET;
  }

  if (s == C_RCV_SET) {
    if (byte == BCC_WR)
      return BCC_OK_SET;
    else if (byte == FLAG_WR)
      return FLAG_RCV_SET;
  }

  if (s == BCC_OK_SET && byte == FLAG_WR)
    return STOP_SET;

  if (s == OTHER_RCV_SET)
    return START_SET;

  return OTHER_RCV_SET;
}

enum states_UA determineState_UA(char byte, enum states_UA s) {
  if (s == START_UA && byte == FLAG_RD)
    return FLAG_RCV_UA;

  if (s == FLAG_RCV_UA) {
    if (byte == A_RD)
      return A_RCV_UA;
    else if (byte == FLAG_RD)
      return FLAG_RCV_UA;
  }

  if (s == A_RCV_UA) {
    if (byte == UA)
      return C_RCV_UA;
    else if (byte == FLAG_RD)
      return FLAG_RCV_UA;
  }

  if (s == C_RCV_UA) {
    if (byte == BCC_RD)
      return BCC_OK_UA;
    else if (byte == FLAG_RD)
      return FLAG_RCV_UA;
  }

  if (s == BCC_OK_UA && byte == FLAG_RD)
    return STOP_UA;

  if (s == OTHER_RCV_UA)
    return START_UA;

  return OTHER_RCV_UA;
}

void printState_SET(enum states_SET s) {
  if (s == START_SET)
    printf("SET Current state: START\n");
  if (s == FLAG_RCV_SET)
    printf("SET Current state: FLAG_RCV\n");
  if (s == A_RCV_SET)
    printf("SET Current state: A_RCV\n");
  if (s == C_RCV_SET)
    printf("SET Current state: C_RCV\n");
  if (s == BCC_OK_SET)
    printf("SET Current state: BCC_OK\n");
  if (s == STOP_SET)
    printf("SET Current state: STOP\n");
  if (s == OTHER_RCV_SET)
    printf("SET Current state: OTHER_RCV\n");
}

void printState_UA(enum states_UA s) {
  if (s == START_UA)
    printf("UA Current state: START\n");
  if (s == FLAG_RCV_UA)
    printf("UA Current state: FLAG_RCV\n");
  if (s == A_RCV_UA)
    printf("UA Current state: A_RCV\n");
  if (s == C_RCV_UA)
    printf("UA Current state: C_RCV\n");
  if (s == BCC_OK_UA)
    printf("UA Current state: BCC_OK\n");
  if (s == STOP_UA)
    printf("UA Current state: STOP\n");
  if (s == OTHER_RCV_UA)
    printf("UA Current state: OTHER_RCV\n");
}
