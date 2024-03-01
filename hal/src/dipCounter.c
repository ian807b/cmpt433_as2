
#include <stdbool.h>
#include <stdlib.h>

#include "hal/sampler.h"
#include "hal/dipCounter.h"

#define THRESHOLD_DIP 0.1
#define HYSTERESIS 0.03

int Dipcounter_counter() {
  bool isInDip = false;
  int numOfDips = 0;
  int historySize = 0;
  double averageReading = 0.0;
  averageReading = Sampler_getAverageReading();
  double* historyCopy = Sampler_getHistory(&historySize);

  for (int i = 0; i < historySize; i++) {
    double voltageDifference = averageReading - historyCopy[i];

    if (!isInDip) {
      if (voltageDifference >= THRESHOLD_DIP) {
        numOfDips++;
        isInDip = true;
      }
    } else {
      if (voltageDifference < (THRESHOLD_DIP - HYSTERESIS)) {
        isInDip = false;
      }
    }
  }

  free(historyCopy);
  return numOfDips;
}