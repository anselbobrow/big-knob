#include "pots.h"
#include "Arduino.h"

void initPots() {
  input = 0;

  pinMode(MPLX_A, OUTPUT);
  pinMode(MPLX_B, OUTPUT);
  pinMode(MPLX_C, OUTPUT);
}

void readPots() {
  digitalWrite(MPLX_A, bitRead(input, 0));
  digitalWrite(MPLX_B, bitRead(input, 1));
  digitalWrite(MPLX_C, bitRead(input, 2));

  int val = analogRead(A0);
  Serial.print(input);
  Serial.print(": ");
  Serial.println(val);
  
  input = (input + 1) % NUM_POTS;
}