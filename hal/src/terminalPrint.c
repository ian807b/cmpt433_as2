#include "hal/terminalPrint.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#include "hal/dipCounter.h"
#include "hal/display.h"
#include "hal/led.h"
#include "hal/periodTimer.h"
#include "hal/sampler.h"
#include "hal/shared.h"

static volatile bool isTerminalThreadRunning = true;
static pthread_t printThread;

void* printThreadFunc(void* arg) {
  (void)arg;

  while (isTerminalThreadRunning) {
    int historySize = 0;
    int numOfDips = 0;
    int potRead = readPOT();

    numOfDips = Dipcounter_counter();
    double* historyCopy = Sampler_getHistory(&historySize);
    Period_statistics_t* period_stats =
        (Period_statistics_t*)malloc(sizeof(Period_statistics_t));
    Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, period_stats);

    if (historySize > 0) {
      printf(
          "#Smpl/s = %d POT @ %d => %dHz avg = %.3fV dips = %d Smpl ms[ %.3f, "
          "%.3f] "
          "avg %.3f/%d samples\n",
          historySize, potRead, getFrequency(potRead),
          Sampler_getAverageReading(), numOfDips, period_stats->minPeriodInMs,
          period_stats->maxPeriodInMs, period_stats->avgPeriodInMs,
          period_stats->numSamples);
      fflush(stdout);

      if (historySize < 20) {
        for (int i = 0; i < historySize; i++) {
          printf("%d:%.3f ", i, historyCopy[i]);
        }
      } else {
        int printSpace = historySize / 20;
        for (int i = 0; i < 20; i++) {
          int printIndex = i * printSpace;
          printf("%d:%.3f ", printIndex, historyCopy[printIndex]);
        }
      }
      printf("\n");
      printf("\n");
      fflush(stdout);
    }
    free(period_stats);
    free(historyCopy);
    sleepForMs(1000);
  }

  return NULL;
}

void Terminalprint_init(void) {
  isTerminalThreadRunning = true;
  pthread_create(&printThread, NULL, printThreadFunc, NULL);
}

void Terminalprint_cleanup(void) {
  isTerminalThreadRunning = false;
  pthread_join(printThread, NULL);
}