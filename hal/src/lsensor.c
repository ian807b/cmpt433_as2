// This is from the last page of "A2DGuide"
#include "hal/lsensor.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define A2D_FILE_VOLTAGE1 "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"
#define A2D_VOLTAGE_REF_V 1.8
#define A2D_MAX_READING 4095

double getVoltage1Reading(void) {
  FILE *f = fopen(A2D_FILE_VOLTAGE1, "r");
  if (!f) {
    printf("ERROR: Unable to open voltage input file. Cape loaded?\n");
    printf("       Check /boot/uEnv.txt for correct options.\n");
    exit(-1);
  }

  int a2dReading = 0;
  int itemsRead = fscanf(f, "%d", &a2dReading);
  if (itemsRead <= 0) {
    printf("ERROR: Unable to read values from voltage input file.\n");
    exit(-1);
  }

  fclose(f);

  double voltage = ((double)a2dReading / A2D_MAX_READING) * A2D_VOLTAGE_REF_V;

  return voltage;
}
