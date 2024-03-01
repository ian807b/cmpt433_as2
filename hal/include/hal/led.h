#ifndef _LED_H_
#define _LED_H_
#include <stdbool.h>

void *flashLED(void *s);
void configureLED(void);
int getFrequency(int rawVal);
void disableLED(void);
void enableLED(void);
int readPOT(void);
void startFlashThread(int hz);
void FlashThreadCleanup(int hz, bool);
void turnOffLED(void);
#endif