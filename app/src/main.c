#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "hal/dipCounter.h"
#include "hal/display.h"
#include "hal/dummy.h"
#include "hal/led.h"
#include "hal/periodTimer.h"
#include "hal/sampler.h"
#include "hal/shared.h"
#include "hal/terminalPrint.h"
#include "hal/udp.h"

bool MAIN_THREAD_FLAG = true;
#define INTERVAL_MS 100 // 100 / 1000 = 10 times a second
// Spawn thread for periodic POT reading
pthread_t POT_THREAD;

/*
Separate function to periodically read POT values and call startFlashThread()
This function is responsible for calling cleanup for the flashing LED cleanup
*/
void *periodicReadPOT(void *arg){
  (void)arg;
  while(MAIN_THREAD_FLAG){
    int POT = readPOT();
    int hz = getFrequency(POT);
    startFlashThread(hz);
    sleepForMs(INTERVAL_MS);
    FlashThreadCleanup(hz, false);
  }
  turnOffLED();
  pthread_exit(NULL);
}

int main() {
  UDP_init();
  Period_init();
  Sampler_init();

  configureLED();
  config_pins();

  turnOnLeftDisplay();
  turnOnRightDisplay();
  Terminalprint_init();

  pthread_create(&POT_THREAD, NULL, periodicReadPOT, NULL);
  // Main application loop
  while (1) {
    // TODO: Replace me with a function that reads POT 10 times a second
    int dips = Dipcounter_counter();
    startDisplayThread(dips);
    sleepForMs(1000);
    DisplayThreadCleanup();
    // Reset the dip counter for the next second or handle as needed
    // DipCounter_reset();  // Implement this if needed to reset state for the
    // next analysis period
    if (!MAIN_THREAD_FLAG) break;
  }
  // Cleanup resources on application termination (not reached in this loop)
  Sampler_cleanup();
  Terminalprint_cleanup();
  UDP_cleanup();
  Period_cleanup();
  pthread_join(POT_THREAD, NULL);
  FlashThreadCleanup(0, true);
  return 0;
}