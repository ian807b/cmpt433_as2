#include "hal/udpMessages.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hal/sampler.h"

#define MAX_PACKET_SIZE 1500
#define SAMPLE_FORMAT_SIZE 8
#define SAMPLES_PER_LINE 10

static int historySize = 0;

char** formatHistoryMessage(int* numOfPackets) {
  double* historyCopy = Sampler_getHistory(&historySize);

  *numOfPackets =
      (int)ceil((double)historySize * SAMPLE_FORMAT_SIZE / MAX_PACKET_SIZE);
  char** packets = (char**)malloc(*numOfPackets * sizeof(char*));

  if (!packets) {
    printf("ERROR: Malloc failed in formatHistoryMessage().\n");
    *numOfPackets = 0;
    return NULL;
  }

  for (int i = 0; i < *numOfPackets; i++) {
    packets[i] = (char*)malloc(MAX_PACKET_SIZE * sizeof(char));
    if (!packets[i]) {
      printf("ERROR: Malloc failed in formatHistoryMessage() for packet %d.\n",
             i);
      for (int j = 0; j < i; j++) free(packets[j]);
      free(packets);
      *numOfPackets = 0;
      return NULL;
    }
    packets[i][0] = '\0';
  }

  int packetNum = 0;
  int charCount = 0;
  int numOfSampleInLine = 0;
  for (int i = 0; i < historySize; i++) {
    char formattedSample[SAMPLE_FORMAT_SIZE];
    if (i == historySize - 1) {
      formattedSample[SAMPLE_FORMAT_SIZE - 1] = '\n';
    }

    if (numOfSampleInLine < SAMPLES_PER_LINE) {
      snprintf(formattedSample, sizeof(formattedSample), "%.3f, ",
               historyCopy[i]);
      numOfSampleInLine++;
    } else {
      snprintf(formattedSample, sizeof(formattedSample), "\n");
      numOfSampleInLine = 0;
    }
    if (charCount + strlen(formattedSample) < MAX_PACKET_SIZE) {
      strcat(packets[packetNum], formattedSample);
      charCount += sizeof(formattedSample);
    } else {
      packetNum++;
      charCount = 0;
      i--;
    }
  }

  strcat(packets[packetNum], "\n");

  free(historyCopy);
  return packets;
}
