/////////////////////// define pins /////////////////
#ifndef PIN_ASSIGNMENT_H
#define PIN_ASSIGNMENT_H

#ifdef M5STACK_ATOM
  #define LED_PIN 27
  #define BTN_PIN 39
#endif

#ifdef M5STAMP_S3
  #define LED_PIN 21
#endif

#endif