#include "hal/led.h"
#include "hal/shared.h"
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <math.h>

#define LED_ENABLE_PATH "/dev/bone/pwm/0/b/enable"
#define A2D_FILE_VOLTAGE_0 "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
volatile bool LED_FLAG = true;
pthread_t LED_THREAD;
volatile bool LED_THREAD_IS_RUNNING = false;
int LAST_HZ_VAL = -1; // -1 is initialization value

void configureLED(void){
    runCommand("config-pin P9_21 pwm");
    runCommand("echo 1000000 > /dev/bone/pwm/0/b/period");
    runCommand("echo 1000000 > /dev/bone/pwm/0/b/duty_cycle");
}

int getFrequency(int rawVal){
    int result = rawVal / 40;
    if(result <= 0){
        result = 1;
    }
    return result;
}

void enableLED(void){
    LED_FLAG = true;
}

void disableLED(void){
    LED_FLAG = false;
}

int readPOT(void){
    FILE *f = fopen(A2D_FILE_VOLTAGE_0, "r");
    if(!f){
        printf("Unable to open voltage input file\n");
        exit(-1);
    }
    int a2dReading = 0;
    int itemsRead = fscanf(f, "%d", &a2dReading);
    if(itemsRead <= 0){
        printf("Error, unable to read values from voltage files\n");
        exit(-1);
    }   
    fclose(f);
    // Hz cannot be zero
    if(a2dReading == 0){
        a2dReading = 1;
    }
    return a2dReading;
}

void turnOffLED(void){
    FILE *led;
    led = fopen(LED_ENABLE_PATH, "w");
    if(led == NULL){
        printf("Error opening LED ENABLE file\n");
        exit(-1);
    }
    writeToFile(led, "0");
    fclose(led);
}




void* flashLED(void *arg){
    int hz = *((int*)arg);
    if (hz == 0) {
        hz = 1;
    }
    FILE *led;
    long long periodMS = 1000/hz;
    while(LED_FLAG){
        led = fopen(LED_ENABLE_PATH, "w");
        if(led == NULL){
            printf("Error opening LED ENABLE file\n");
            exit(-1);
        }
        // turn on LED
        writeToFile(led, "1");
        fclose(led);
        // turn off LED
        sleepForMs(periodMS / 2);

        led = fopen(LED_ENABLE_PATH, "w");
        if(led == NULL){
            printf("Error opening LED ENABLE file\n");
            exit(-1);
        }
        // turn off LED
        writeToFile(led, "0");
        fclose(led);
        sleepForMs(periodMS / 2);
    }
    turnOffLED();
    return NULL;
}

void startFlashThread(int hz){
    // Can only have 1 LED thread running
    if(!LED_THREAD_IS_RUNNING && hz != LAST_HZ_VAL){
        enableLED();
        LED_THREAD_IS_RUNNING = true;
        int ret = pthread_create(&LED_THREAD, NULL, flashLED, (void *)&hz);
        if(ret != 0){
            printf("Error, LED_THREAD returned: %d\n", ret);
            exit(-1);
        }
        LAST_HZ_VAL = hz;
    }
}

void FlashThreadCleanup(int hz, bool isShutDown){
    if(LED_THREAD_IS_RUNNING){
        if(hz != LAST_HZ_VAL && !isShutDown){
            disableLED();
            pthread_join(LED_THREAD, NULL);
            LED_THREAD_IS_RUNNING = false;
        }else if(isShutDown){ // This branch should only be taken at end of main()
            disableLED();
            pthread_join(LED_THREAD, NULL);
            LED_THREAD_IS_RUNNING = false;
        }
    }
}