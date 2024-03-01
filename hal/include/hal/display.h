#ifndef _DISPLAY_H_
#define _DISPLAY_H_

struct display_thread_data{
    int leftA;
    int leftB;
    int rightA;
    int rightB;
};

void config_pins(void);
void turnOnLeftDisplay(void);
void turnOnRightDisplay(void);
void turnOffLeftDisplay(void);
void turnOffRightDisplay(void);
void getDigits(int, int *);
void enableDisplayThread(void);
void stopDisplayThread(void);
void mapDisplayVal(int left, int right);
void *display(void *);
void startDisplayThread(int);
void DisplayThreadCleanup(void);
#endif