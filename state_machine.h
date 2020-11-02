#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

// I, SET, DISC - Comandos
// UA, RR, REJ - Respostas

#define A_CA 0x03 // A - Campo de Endereço em Comandos enviados pelo Emissor e Respostas enviadas pelo Recetor
#define A_AC 0x01  // A - Campo de Endereço em Respostas enviados pelo Emissor e Comandos enviadas pelo Recetor

#define FLAG_SET 0x7e // Flag do comando SET
#define SET 0x03 // Campo de Controlo do SET
#define BCC_SET (A_CA^SET) // Campo de Proteção (Cabeçalho) do SET

#define FLAG_UA 0x7e // Flag da resposta UA
#define UA 0x07 // Campo de Controlo do UA
#define BCC_UA_TRANSMITTER (A_AC^UA) // Campo de Proteção (Cabeçalho) do UA
#define BCC_UA_RECEIVER (A_CA^UA) // Campo de Proteção (Cabeçalho) do UA

#define FLAG_I 0x7e // Flag do comando I
#define I0 0x00 // Campo de Controlo do I com S = 0
#define I1 0x40 // Campo de Controlo do I com S = 1
#define BCC1_I0 (A_CA^I0) // Campo de Proteção (Cabeçalho) do I com S = 0
#define BCC1_I1 (A_CA^I1) // Campo de Proteção (Cabeçalho) do I com S = 1

#define FLAG_RR 0x7e // Flag da resposta RR
#define RR0 0x05 // Campo de Controlo do RR com R = 0
#define RR1 0x85 // Campo de Controlo do RR com R = 1
#define BCC_RR0 (A_CA^RR0)  // Campo de Proteção (Cabeçalho) do RR com R = 0
#define BCC_RR1 (A_CA^RR1)  // Campo de Proteção (Cabeçalho) do RR com R = 1

#define FLAG_REJ 0x7e // Flag da resposta REJ
#define REJ0 0x01 // Campo de Controlo do REJ com R = 0
#define REJ1 0x81 // Campo de Controlo do REJ com R = 1
#define BCC_REJ0 (A_CA^REJ0)  // Campo de Proteção (Cabeçalho) do REJ com R = 0
#define BCC_REJ1 (A_CA^REJ1)  // Campo de Proteção (Cabeçalho) do REJ com R = 1

#define FLAG_DISC 0x7e // Flag do comando DISC
#define DISC 0x07 // Campo de Controlo do DISC
#define BCC_DISC_TRANSMITTER (A_CA^DISC) // Campo de Proteção (Cabeçalho) do DISC
#define BCC_DISC_RECEIVER (A_AC^DISC) // Campo de Proteção (Cabeçalho) do DISC

#define ESCAPE 0x7d
#define PATTERNFLAG 0x5e
#define PATTERNESCAPE 0x5d

enum states_SET { // Estados da Maquina de Estados do comando SET
  START_SET,
  FLAG_RCV_SET,
  A_RCV_SET,
  C_RCV_SET,
  BCC_OK_SET,
  STOP_SET,
  OTHER_RCV_SET
} state_SET;

enum states_UA { // Estados da Maquina de Estados da resposta UA
  START_UA,
  FLAG_RCV_UA,
  A_RCV_UA,
  C_RCV_UA,
  BCC_OK_UA,
  STOP_UA,
  OTHER_RCV_UA
} state_UA;

enum states_I { // Estados da Maquina de Estados do comando I
  START_I,
  FLAG_RCV_I,
  A_RCV_I,
  C_RCV_I,
  BCC1_OK_I,
  D_RCV_I,
  BCC2_OK_I,
  STOP_I,
  OTHER_RCV_I
} state_I;

enum states_RR_REJ { // Estados da Maquina de Estados das respostas RR e REJ
  START_RR_REJ,
  FLAG_RCV_RR_REJ,
  A_RCV_RR_REJ,
  C_RCV_RR,
  C_RCV_REJ,
  BCC_OK_RR,
  BCC_OK_REJ,
  STOP_RR_REJ,
  OTHER_RCV_RR_REJ
} state_RR_REJ;

enum states_DISC { // Estados da Maquina de Estados do comando DISC
  START_DISC,
  FLAG_RCV_DISC,
  A_RCV_DISC,
  C_RCV_DISC,
  BCC_OK_DISC,
  STOP_DISC,
  OTHER_RCV_DISC
} state_DISC;

enum states_SET determineState_SET(unsigned char byte, enum states_SET s); // Determinador de estados da Máquina de Estados do comando SET

enum states_UA determineState_UA(unsigned char byte, enum states_UA s); // Determinador de estados da Máquina de Estados da resposta UA

enum states_I determineState_I(unsigned char byte, enum states_I s); // Determinador de estados da Máquina de Estados do comando I

enum states_RR_REJ determineState_RR_REJ(unsigned char byte, enum states_RR_REJ s); // Determinador de estados da Máquina de Estados das respostas RR e REJ

//enum states_RR determineState_RR(unsigned char byte, enum states_RR s); // Determinador de estados da Máquina de Estados da resposta RR

//enum states_REJ determineState_REJ(unsigned char byte, enum states_REJ s); // Determinador de estados da Máquina de Estados da resposta REJ

enum states_DISC determineState_DISC(unsigned char byte, enum states_DISC s); // Determinador de estados da Máquina de Estados do comando DISC

void printState_SET(enum states_SET s); // Função auxiliar que imprime o Estado s da Máquina de Estados do comando SET

void printState_UA(enum states_UA s); // Função auxiliar que imprime o Estado s da Máquina de Estados da resposta UA

void printState_I(enum states_I s); // Função auxiliar que imprime o Estado s da Máquina de Estados do comando I

void printState_RR_REJ(enum states_RR_REJ s); // Função auxiliar que imprime o Estado s da Máquina de Estados das respostas RR e REJ

//void printState_RR(enum states_RR s); // Função auxiliar que imprime o Estado s da Máquina de Estados da resposta RR

//void printState_REJ(enum states_REJ s); // Função auxiloar que imprime o Estado s da Máquina de Estados da resposta REJ

void printState_DISC(enum states_DISC s); // Função auxiliar que imprime o Estado s da Máquina de Estados do comando DISC
