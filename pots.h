#ifndef H_POTS
#define H_POTS

#include "Arduino.h"

// pin defs
#define MPLX_A 10
#define MPLX_B 11
#define MPLX_C 12

#define NUM_POTS 4

static int input;

void initPots();
void readPots();

#endif // #ifndef H_POTS