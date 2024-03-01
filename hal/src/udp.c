#include "hal/udp.h"

#include <ctype.h>
#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "hal/dipCounter.h"
#include "hal/sampler.h"
#include "hal/shared.h"
#include "hal/udpMessages.h"

#define MAX_LEN 1500
#define PORT 12345

static int socketDescriptor;
static struct sockaddr_in sin, sinRemote;
static unsigned int sin_len = sizeof(sinRemote);
static pthread_t udpThread;
static volatile bool isUdpThreadRunning = true;
static Command lastCommand = COMMAND_UNKNOWN;
static char commandRcvd[MAX_LEN];
static char returnMessage[MAX_LEN];
static char commandInLowerCase[MAX_LEN];

void processCommand(Command parsedCommand) {
  switch (parsedCommand) {
    case COMMAND_HELP:
      snprintf(returnMessage, MAX_LEN,
               "Accepted commands:\n"
               "count\t\t-- get the total number of samples taken.\n"
               "length\t\t-- get the total number of samples taken in the "
               "previously "
               "completed second.\ndips\t\t-- get the number of dips in the "
               "previously completed "
               "second.\nhistory\t\t-- get all the samples in the previously "
               "completed "
               "second.\nstop\t\t-- cause the server program to "
               "end.\n<enter>\t\t-- repeat last "
               "command.\n");
      break;
    case COMMAND_COUNT:
      snprintf(returnMessage, MAX_LEN, "# of samples taken total: %lld\n",
               Sampler_getNumSamplesTaken());
      break;
    case COMMAND_LENGTH:
      snprintf(returnMessage, MAX_LEN, "# samples taken last second: %d\n",
               Sampler_getHistorySize());
      break;
    case COMMAND_DIPS:
      snprintf(returnMessage, MAX_LEN, "# Dips: %d\n", Dipcounter_counter());
      break;
    case COMMAND_HISTORY: {
      int numOfPackets = 0;
      char** packets = formatHistoryMessage(&numOfPackets);

      for (int i = 0; i < numOfPackets; i++) {
        sendto(socketDescriptor, packets[i], strlen(packets[i]), 0,
               (struct sockaddr*)&sinRemote, sin_len);
        free(packets[i]);
      }

      free(packets);

      if (numOfPackets == 0) {
        snprintf(returnMessage, MAX_LEN,
                 "No history data to send. Please check the light sensor.\n");
      }

      if (parsedCommand != COMMAND_ENTER) {
        lastCommand = parsedCommand;
      }

      return;
    }
    case COMMAND_STOP:
      snprintf(returnMessage, MAX_LEN, "Program terminating...\n");
      sendto(socketDescriptor, returnMessage, strlen(returnMessage), 0,
             (struct sockaddr*)&sinRemote, sin_len);
      UDP_cleanup();
      return;
    case COMMAND_ENTER:
      if (lastCommand != COMMAND_UNKNOWN && lastCommand != COMMAND_ENTER) {
        processCommand(lastCommand);
        return;
      }
      snprintf(returnMessage, MAX_LEN, "Unknown command.\n");
      break;
    default:
      snprintf(returnMessage, MAX_LEN, "Unknown command.\n");
      break;
  }

  if (parsedCommand != COMMAND_ENTER) {
    lastCommand = parsedCommand;
  }

  sendto(socketDescriptor, returnMessage, strlen(returnMessage), 0,
         (struct sockaddr*)&sinRemote, sin_len);
}

Command parseCommand(const char* commandRcvd) {
  for (size_t i = 0; i < strlen(commandRcvd) + 1; i++) {
    commandInLowerCase[i] = tolower(commandRcvd[i]);
  }

  int i = 0;
  while (commandInLowerCase[i] != '\0') {
    if (commandInLowerCase[i] == '\n') {
      commandInLowerCase[i] = '\0';
      break;
    }
    i++;
  }

  if (strcmp(commandInLowerCase, "help") == 0 ||
      strcmp(commandInLowerCase, "?") == 0)
    return COMMAND_HELP;
  if (strcmp(commandInLowerCase, "count") == 0) return COMMAND_COUNT;
  if (strcmp(commandInLowerCase, "length") == 0) return COMMAND_LENGTH;
  if (strcmp(commandInLowerCase, "dips") == 0) return COMMAND_DIPS;
  if (strcmp(commandInLowerCase, "history") == 0) return COMMAND_HISTORY;
  if (strcmp(commandInLowerCase, "stop") == 0) return COMMAND_STOP;
  if (strcmp(commandInLowerCase, "") == 0) return COMMAND_ENTER;

  return COMMAND_UNKNOWN;
}

void* udpThreadFunc(void* arg) {
  (void)arg;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(PORT);

  socketDescriptor = socket(PF_INET, SOCK_DGRAM, 0);
  bind(socketDescriptor, (struct sockaddr*)&sin, sizeof(sin));

  while (isUdpThreadRunning) {
    int bytesRx = recvfrom(socketDescriptor, commandRcvd, MAX_LEN - 1, 0,
                           (struct sockaddr*)&sinRemote, &sin_len);
    commandRcvd[bytesRx] = '\0';
    Command parsedCommand = parseCommand(commandRcvd);
    processCommand(parsedCommand);
  }

  return NULL;
}

void UDP_init(void) {
  isUdpThreadRunning = true;
  pthread_create(&udpThread, NULL, udpThreadFunc, NULL);
}

void UDP_cleanup(void) {
  isUdpThreadRunning = false;
  pthread_join(udpThread, NULL);
  close(socketDescriptor);
  MAIN_THREAD_FLAG = false;
}