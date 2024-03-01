#ifndef UDP_H
#define UDP_H

typedef enum {
  COMMAND_HELP,
  COMMAND_COUNT,
  COMMAND_LENGTH,
  COMMAND_DIPS,
  COMMAND_HISTORY,
  COMMAND_STOP,
  COMMAND_ENTER,
  COMMAND_UNKNOWN,
} Command;

void UDP_init(void);
void UDP_cleanup(void);

#endif
