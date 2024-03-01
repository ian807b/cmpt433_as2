
#include "hal/sampler.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hal/lsensor.h"
#include "hal/shared.h"
#include "hal/periodTimer.h"

#define HISTORY_SIZE 1000

static pthread_t sampler;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static double sampleHistory[HISTORY_SIZE] = {0.0};
static double currentHistory[HISTORY_SIZE] = {0.0};
static int currentHistoryIndex = 0;
static atomic_llong totalSampleCount = ATOMIC_VAR_INIT(0);
static atomic_int sampleHistoryCount = ATOMIC_VAR_INIT(0);
static double averageVoltageReading = 0.0;
static bool isFirstSample = true;
static volatile bool isSamplerThreadRunning = true;

// Measuring time referenced from:
// https://blog.naver.com/0820b/223127869314
// https://stackoverflow.com/questions/64893834/measuring-elapsed-time-using-clock-gettimeclock-monotonic
void* threadFunc(void* arg) {
  (void)arg;
  struct timespec startTime, currentTime;

  clock_gettime(CLOCK_MONOTONIC, &startTime);

  while (isSamplerThreadRunning) {
    pthread_mutex_lock(&mutex);
    if (currentHistoryIndex < HISTORY_SIZE) {
      double newSample = getVoltage1Reading();
      Period_markEvent(PERIOD_EVENT_SAMPLE_LIGHT);
      currentHistory[currentHistoryIndex++] = newSample;
      atomic_fetch_add(&totalSampleCount, 1);

      if (isFirstSample) {
        averageVoltageReading = newSample;
        isFirstSample = false;
      }

      averageVoltageReading = 0.001 * newSample + 0.999 * averageVoltageReading;
    }
    pthread_mutex_unlock(&mutex);
    sleepForMs(1);

    clock_gettime(CLOCK_MONOTONIC, &currentTime);
    long timeElapsedInMs = (currentTime.tv_sec - startTime.tv_sec) * 1000 +
                           (currentTime.tv_nsec - startTime.tv_nsec) / 1000000;

    if (timeElapsedInMs >= 1000) {
      Sampler_moveCurrentDataToHistory();
      startTime = currentTime;
    }
  }

  return NULL;
}

void Sampler_init(void) {
  isSamplerThreadRunning = true;
  pthread_create(&sampler, NULL, threadFunc, NULL);
}

void Sampler_cleanup(void) {
  isSamplerThreadRunning = false;
  pthread_join(sampler, NULL);
}

void Sampler_moveCurrentDataToHistory(void) {
  pthread_mutex_lock(&mutex);
  memcpy(sampleHistory, currentHistory, currentHistoryIndex * sizeof(double));
  atomic_store(&sampleHistoryCount, currentHistoryIndex);
  currentHistoryIndex = 0;
  pthread_mutex_unlock(&mutex);
}

int Sampler_getHistorySize(void) { return atomic_load(&sampleHistoryCount); }

double* Sampler_getHistory(int* size) {
  pthread_mutex_lock(&mutex);
  *size = Sampler_getHistorySize();
  double* copy = malloc(*size * sizeof(double));
  if (copy) {
    memcpy(copy, sampleHistory, *size * sizeof(double));
  } else {
    printf("ERROR: Malloc failed in Sampler_getHistory().\n");
    free(copy);
    return NULL;
  }
  pthread_mutex_unlock(&mutex);

  return copy;
}

double Sampler_getAverageReading(void) {
  double ret = 0.0;
  pthread_mutex_lock(&mutex);
  ret = averageVoltageReading;
  pthread_mutex_unlock(&mutex);
  return ret;
}

long long Sampler_getNumSamplesTaken(void) {
  return atomic_load(&totalSampleCount);
}