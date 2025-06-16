#ifndef REMINDER_H
#define REMINDER_H

#include <time.h>

#define SOCKET_PATH "/tmp/reminder.sock"
#define BUFFER_SIZE 1024
#define BACKLOG     10

/* Reminder struct */
typedef struct {
  time_t time;
  char message[BUFFER_SIZE];
} Reminder;

#endif // REMINDER_H
