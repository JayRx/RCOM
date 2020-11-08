#include "state_machine.h"

enum states_SET determineState_SET(unsigned char byte, enum states_SET s) {
  if (s == START_SET && byte == FLAG_SET)
    return FLAG_RCV_SET;

  if (s == FLAG_RCV_SET) {
    if (byte == A_CA)
      return A_RCV_SET;
    else if (byte == FLAG_SET)
      return FLAG_RCV_SET;
  }

  if (s == A_RCV_SET) {
    if (byte == SET)
      return C_RCV_SET;
    else if (byte == FLAG_SET)
      return FLAG_RCV_SET;
  }

  if (s == C_RCV_SET) {
    if (byte == BCC_SET)
      return BCC_OK_SET;
    else if (byte == FLAG_SET)
      return FLAG_RCV_SET;
  }

  if (s == BCC_OK_SET && byte == FLAG_SET)
    return STOP_SET;

  if (s == OTHER_RCV_SET)
    return START_SET;

  return START_SET;
}

enum states_UA determineState_UA(unsigned char byte, enum states_UA s) {
  if (s == START_UA && byte == FLAG_UA)
    return FLAG_RCV_UA;

  if (s == FLAG_RCV_UA) {
    if (byte == A_CA || byte == A_AC)
      return A_RCV_UA;
    else if (byte == FLAG_UA)
      return FLAG_RCV_UA;
  }

  if (s == A_RCV_UA) {
    if (byte == UA)
      return C_RCV_UA;
    else if (byte == FLAG_UA)
      return FLAG_RCV_UA;
  }

  if (s == C_RCV_UA) {
    if (byte == BCC_UA_TRANSMITTER || byte == BCC_UA_RECEIVER)
      return BCC_OK_UA;
    else if (byte == FLAG_UA)
      return FLAG_RCV_UA;
  }

  if (s == BCC_OK_UA && byte == FLAG_UA)
    return STOP_UA;

  if (s == OTHER_RCV_UA)
    return START_UA;

  return START_UA;
}

enum states_I determineState_I(unsigned char byte, enum states_I s) {
  if (s == START_I && byte == FLAG_I)
    return FLAG_RCV_I;

  if (s == FLAG_RCV_I) {
    if (byte == A_CA)
      return A_RCV_I;
    else if (byte == FLAG_I)
      return FLAG_RCV_I;
  }

  if (s == A_RCV_I) {
    if (byte == I0 || byte == I1)
      return C_RCV_I;
    else if (byte == FLAG_I)
      return FLAG_RCV_I;
  }

  if (s == C_RCV_I) {
    if (byte == BCC1_I0 || byte == BCC1_I1)
      return BCC1_OK_I;
  }

  if (s == BCC1_OK_I) {
    if (byte == FLAG_I)
      return STOP_I;
    else
      return D_RCV_I;
  }

  if (s == D_RCV_I) {
    if (byte == FLAG_I)
      return STOP_I;
    else
      return D_RCV_I;
  }

  if (s == BCC2_OK_I && byte == FLAG_I)
    return STOP_I;

  if (s == OTHER_RCV_I)
    return START_I;

  return START_I;
}

enum states_RR_REJ determineState_RR_REJ(unsigned char byte, enum states_RR_REJ s) {
  if (s == START_RR_REJ && (byte == FLAG_RR || byte == FLAG_REJ))
    return FLAG_RCV_RR_REJ;

  if (s == FLAG_RCV_RR_REJ) {
    if (byte == A_CA)
      return A_RCV_RR_REJ;
    else if (byte == FLAG_RR || byte == FLAG_REJ)
      return FLAG_RCV_RR_REJ;
  }

  if (s == A_RCV_RR_REJ) {
    if (byte == RR0 || byte == RR1)
      return C_RCV_RR;
    else if (byte == REJ0 || byte == REJ1)
      return C_RCV_REJ;
    else if (byte == FLAG_RR || byte == FLAG_REJ)
      return FLAG_RCV_RR_REJ;
  }

  if (s == C_RCV_RR) {
    if (byte == BCC_RR0 || byte == BCC_RR1)
      return BCC_OK_RR;
    else if (byte == FLAG_RR || byte == FLAG_REJ)
      return FLAG_RCV_RR_REJ;
  }

  if (s == C_RCV_REJ) {
    if (byte == BCC_REJ0 || byte == BCC_REJ1)
      return BCC_OK_REJ;
    else if (byte == FLAG_RR || byte == FLAG_REJ)
      return FLAG_RCV_RR_REJ;
  }

  if (s == BCC_OK_RR && byte == FLAG_RR)
    return STOP_RR_REJ;

  if (s == BCC_OK_REJ && byte == FLAG_REJ)
    return STOP_RR_REJ;

  if (s == OTHER_RCV_RR_REJ)
    return START_RR_REJ;

  return START_RR_REJ;
}

enum states_DISC determineState_DISC(unsigned char byte, enum states_DISC s) {
  if (s == START_DISC && byte == FLAG_DISC)
    return FLAG_RCV_DISC;

  if (s == FLAG_RCV_DISC) {
    if (byte == A_CA || byte == A_AC)
      return A_RCV_DISC;
    else if (byte == FLAG_DISC)
      return FLAG_RCV_DISC;
  }

  if (s == A_RCV_DISC) {
    if (byte == DISC)
      return C_RCV_DISC;
    else if (byte == FLAG_DISC)
      return FLAG_RCV_DISC;
  }

  if (s == C_RCV_DISC) {
    if (byte == BCC_DISC_TRANSMITTER || byte == BCC_DISC_RECEIVER)
      return BCC_OK_DISC;
    else if (byte == FLAG_DISC)
      return FLAG_RCV_DISC;
  }

  if (s == BCC_OK_DISC && byte == FLAG_DISC)
    return STOP_DISC;

  if (s == OTHER_RCV_DISC)
    return START_DISC;

  return START_DISC;
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

void printState_I(enum states_I s) {
  if (s == START_I)
    printf("I Current state: START\n");
  if (s == FLAG_RCV_I)
    printf("I Current state: FLAG_RCV\n");
  if (s == A_RCV_I)
    printf("I Current state: A_RCV\n");
  if (s == C_RCV_I)
    printf("I Current state: C_RCV\n");
  if (s == BCC1_OK_I)
    printf("I Current state: BCC1_OK\n");
  if (s == D_RCV_I)
    printf("I Current state: D_RCV\n");
  if (s == BCC2_OK_I)
    printf("I Current state: BCC2_OK\n");
  if (s == STOP_I)
    printf("I Current state: STOP\n");
  if (s == OTHER_RCV_I)
    printf("I Current state: OTHER_RCV\n");
}

void printState_RR_REJ(enum states_RR_REJ s) {
  if (s == START_RR_REJ)
    printf("RR_REJ Current state: START\n");
  if (s == FLAG_RCV_RR_REJ)
    printf("RR_REJ Current state: FLAG_RCV\n");
  if (s == A_RCV_RR_REJ)
    printf("RR_REJ Current state: A_RCV\n");
  if (s == C_RCV_RR)
    printf("RR Current state: C_RCV\n");
  if (s == C_RCV_REJ)
    printf("REJ Current state: C_RCV\n");
  if (s == BCC_OK_RR)
    printf("RR Current state: BCC_OK\n");
  if (s == BCC_OK_REJ)
    printf("REJ Current state: BCC_OK\n");
  if (s == STOP_RR_REJ)
    printf("RR_REJ Current state: STOP\n");
  if (s == OTHER_RCV_RR_REJ)
    printf("RR_REJ Current state: OTHER_RCV\n");
}

void printState_DISC(enum states_DISC s) {
  if (s == START_DISC)
    printf("DISC Current state: START\n");
  if (s == FLAG_RCV_DISC)
    printf("DISC Current state: FLAG_RCV\n");
  if (s == A_RCV_DISC)
    printf("DISC Current state: A_RCV\n");
  if (s == C_RCV_DISC)
    printf("DISC Current state: C_RCV\n");
  if (s == BCC_OK_DISC)
    printf("DISC Current state: BCC_OK\n");
  if (s == STOP_DISC)
    printf("DISC Current state: STOP\n");
  if (s == OTHER_RCV_DISC)
    printf("DISC Current state: OTHER_RCV\n");
}
