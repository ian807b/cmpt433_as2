#include "hal/display.h"
#include "hal/shared.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>

#define I2CDRV_LINUX_BUS0 "/dev/i2c-0"
#define I2CDRV_LINUX_BUS1 "/dev/i2c-1"
#define I2CDRV_LINUX_BUS2 "/dev/i2c-2"
#define LEFT_VALUE_PATH "/sys/class/gpio/gpio61/value"
#define RIGHT_VALUE_PATH "/sys/class/gpio/gpio44/value"


static int REG_DIRA = 0x02;
static int REG_DIRB = 0x03;
static int REG_OUTA = 0x00; // right hemisphere
static int REG_OUTB = 0x01; // left hemisphere

/*
Hex values for Left and Right hemispheres, corresponding to a number
Use these when trying to display a number, REGA and REGB must be used in tandem
*/
static int REG_OUTA_0 = 0xD0;
static int REG_OUTA_1 = 0x02;
static int REG_OUTA_2 =0x98;
static int REG_OUTA_3 =0xD8;
static int REG_OUTA_4 =0xC8;
static int REG_OUTA_5 =0x58;
static int REG_OUTA_6 =0x58;
static int REG_OUTA_7 =0xC0;
static int REG_OUTA_8 =0xD8;
static int REG_OUTA_9 =0xC8;


static int REG_OUTB_0 =0xA1;
static int REG_OUTB_1 =0x08;
static int REG_OUTB_2 =0x83;
static int REG_OUTB_3 =0x03;
static int REG_OUTB_4 =0x22;
static int REG_OUTB_5 =0x23;
static int REG_OUTB_6 =0xA3;
static int REG_OUTB_7 =0x01;
static int REG_OUTB_8 =0xA3;
static int REG_OUTB_9 = 0x23;

volatile bool runDisplay = true;
volatile bool isDisplayThreadRunning = false;
pthread_t DISPLAY_THREAD;
int buff[2] = {0,0};
struct display_thread_data *my_data;

void stopDisplayThread(void){
    runDisplay = false;
}

void enableDisplayThread(void){
    runDisplay = true;
}

void config_pins(void){
    runCommand("echo out > /sys/class/gpio/gpio61/direction");
    runCommand("echo out > /sys/class/gpio/gpio44/direction");
    runCommand("config-pin P9_18 i2c");
    runCommand("config-pin P9_17 i2c");
}

void turnOnLeftDisplay(void){
    runCommand("echo 1 > /sys/class/gpio/gpio61/value");
}
void turnOnRightDisplay(void){
    runCommand("echo 1 > /sys/class/gpio/gpio44/value");
}

void turnOffLeftDisplay(void){
    runCommand("echo 0 > /sys/class/gpio/gpio61/value");
}

void turnOffRightDisplay(void){
    runCommand("echo 0 > /sys/class/gpio/gpio44/value");
}

void getDigits(int var, int* output){
    size_t n = sizeof(output);
    if(n < 2){
        printf("Error, output array is too small for getDigits\n");
        exit(-1);
    }
    int num = var;
    int it = 0;

    while(it <= 1){
        int mod = num % 10;
        output[it] = mod;
        num = num/10;
        it++;
    }
}

void mapDisplayVal(int left, int right){
    switch(left){
        case 0:
            my_data->leftA = REG_OUTA_0;
            my_data->leftB = REG_OUTB_0;
            break;
        case 1: 
            my_data->leftA = REG_OUTA_1;
            my_data->leftB = REG_OUTB_1;
            break;
        case 2:
            my_data->leftA = REG_OUTA_2;
            my_data->leftB = REG_OUTB_2;
            break;
        case 3:
            my_data->leftA = REG_OUTA_3;
            my_data->leftB = REG_OUTB_3;
            break;
        case 4:
            my_data->leftA = REG_OUTA_4;
            my_data->leftB = REG_OUTB_4;
            break;
        case 5:
            my_data->leftA = REG_OUTA_5;
            my_data->leftB = REG_OUTB_5;
            break;
        case 6:
            my_data->leftA = REG_OUTA_6;
            my_data->leftB = REG_OUTB_6;
            break;
        case 7:
            my_data->leftA = REG_OUTA_7;
            my_data->leftB = REG_OUTB_7;
            break;
        case 8:
            my_data->leftA = REG_OUTA_8;
            my_data->leftB = REG_OUTB_8;
            break;
        case 9:
            my_data->leftA = REG_OUTA_9;
            my_data->leftB = REG_OUTB_9;
            break;
        default:
            printf("Error, unknown integer: %d\n", left);
            exit(-1);
    }
    switch(right){
        case 0:
            my_data->rightA = REG_OUTA_0;
            my_data->rightB = REG_OUTB_0;
            break;
        case 1: 
            my_data->rightA = REG_OUTA_1;
            my_data->rightB = REG_OUTB_1;
            break;
        case 2:
            my_data->rightA = REG_OUTA_2;
            my_data->rightB = REG_OUTB_2;
            break;
        case 3:
            my_data->rightA = REG_OUTA_3;
            my_data->rightB = REG_OUTB_3;
            break;
        case 4:
            my_data->rightA = REG_OUTA_4;
            my_data->rightB = REG_OUTB_4;
            break;
        case 5:
            my_data->rightA = REG_OUTA_5;
            my_data->rightB = REG_OUTB_5;
            break;
        case 6:
            my_data->rightA = REG_OUTA_6;
            my_data->rightB = REG_OUTB_6;
            break;
        case 7:
            my_data->rightA = REG_OUTA_7;
            my_data->rightB = REG_OUTB_7;
            break;
        case 8:
            my_data->rightA = REG_OUTA_8;
            my_data->rightB = REG_OUTB_8;
            break;
        case 9:
            my_data->rightA = REG_OUTA_9;
            my_data->rightB = REG_OUTB_9;
            break;
        default:
            printf("Error, unknown integer: %d\n", left);
            exit(-1);
    }
}

void *display(void *arg){
    (void)arg;
    int i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, 0x20);
    if(i2cFileDesc < 0){
        printf("Error, i2cFileDesc returned %d\n", i2cFileDesc);
        exit(-1);
    }

    writeI2cReg(i2cFileDesc, REG_DIRA, 0x00);
    writeI2cReg(i2cFileDesc, REG_DIRB, 0x00);
    FILE *left, *right;
    while(runDisplay){
        left = fopen(LEFT_VALUE_PATH, "w");
        right = fopen(RIGHT_VALUE_PATH, "w");

        if(left == NULL || right == NULL){
            printf("Error opening files for GPIO44/61\n");
            exit(-1);
        }
        // turn off right and left displays
        writeToFile(left, "0");
        writeToFile(right, "0");
        fclose(left);
        fclose(right);
        // Write to REG_OUTA/B
        // Do this locally for faster performance
        // Write to Reg addr: 0x00
        unsigned char charbuff[2];
        charbuff[0] = REG_OUTA;
        charbuff[1] = my_data->leftA;
        int res = write(i2cFileDesc, charbuff, 2);
        if( res != 2){
            perror("I2C: Unable to write i2c register.");
            exit(1);
        }
        // write to Reg addr: 0x01
        charbuff[0] = REG_OUTB;
        charbuff[1] = my_data->leftB;
        res = write(i2cFileDesc, charbuff, 2);
        if( res != 2){
            perror("I2C: Unable to write i2c register.");
            exit(1);
        }

        left = fopen(LEFT_VALUE_PATH, "w");
        if(left == NULL){
            printf("Error opening files for GPIO44/61\n");
            exit(-1);
        }
        // turn on left
        writeToFile(left, "1");
        fclose(left);
        sleepForMs(5);

        // turn off left
        left = fopen(LEFT_VALUE_PATH, "w");
        if(left == NULL){
            printf("Error opening files for GPIO44/61\n");
            exit(-1);
        }
        writeToFile(left, "0");
        fclose(left);

        // Write second number
        charbuff[0] = REG_OUTA;
        charbuff[1] = my_data->rightA;
        res = write(i2cFileDesc, charbuff, 2);
        if( res != 2){
            perror("I2C: Unable to write i2c register.");
            exit(1);
        }

        charbuff[0] = REG_OUTB;
        charbuff[1] = my_data->rightB;
        res = write(i2cFileDesc, charbuff, 2);
        if( res != 2){
            perror("I2C: Unable to write i2c register.");
            exit(1);
        }

        //turn on right
        right = fopen(RIGHT_VALUE_PATH, "w");
        if(right == NULL){
            printf("Error opening files for GPIO44/61\n");
            exit(-1);
        }
        writeToFile(right, "1");
        fclose(right);
        sleepForMs(5);
    }

    //turnOffLeftDisplay();
    //turnOffRightDisplay();
    writeI2cReg(i2cFileDesc, REG_OUTA, 0x00);
    writeI2cReg(i2cFileDesc, REG_OUTB, 0x00);
    close(i2cFileDesc);
    return NULL;
}

void startDisplayThread(int dips){
    if(!isDisplayThreadRunning){
        my_data = malloc(sizeof(struct display_thread_data));
        if (my_data == NULL) {
            printf("Error, could not allocate memory for my_data\n");
            exit(-1);
        }
        
        getDigits(dips, buff);
        mapDisplayVal(buff[1], buff[0]);
        stopDisplayThread();
        enableDisplayThread();
        int ret = pthread_create(&DISPLAY_THREAD, NULL, display, NULL);
        if(ret != 0){
            printf("Error, disp_thread returned: %d\n", ret);
            exit(-1);
        }
        isDisplayThreadRunning = true;
    }
}

void DisplayThreadCleanup(void){
    if(isDisplayThreadRunning){
        stopDisplayThread();
        pthread_join(DISPLAY_THREAD, NULL);
        turnOffLeftDisplay();
        turnOffRightDisplay();
        free(my_data);
        isDisplayThreadRunning = false;
    }
}
