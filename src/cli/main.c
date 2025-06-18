#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "../headers/reminder.h"

void usage()
{
  puts("Usage: remider <string> <time>");
  puts("Example: reminder \"Math\" 13:00");
}

/* Convert HH:MM time to time_t */
int parse_time(const char *input, time_t *out_time)
{
  struct tm t   = {0};
  time_t now    = time(NULL);

  localtime_r(&now, &t);

  if (sscanf(input, "%2d:%2d", &t.tm_hour, &t.tm_min) != 2)
    return -1;

  t.tm_sec = 0;
  time_t result = mktime(&t);
  if (result == -1) return -1;

  *out_time = result;
  return 0;
}

int main(int argc, char **argv)
{
  int       clientfd;
  struct    sockaddr_un addr;

  char      buffer[BUFFER_SIZE];
  socklen_t addrlen = sizeof(struct sockaddr_un);

  /* Arguments */
  if (argc < 3) {
    usage();
    return EXIT_FAILURE;
  }

  const char *msg   = argv[1];
  const char *time_str  = argv[2];

  time_t ttime;
  if (parse_time(time_str, &ttime) == -1) {
    puts("Invalid time format");
    return EXIT_FAILURE;
  }

  clientfd = socket(AF_UNIX, SOCK_STREAM, 0); 
  if (clientfd == -1) {
    perror("socket");
    return EXIT_FAILURE;
  }

  /* Address */
  memset(&addr, 0, addrlen);
  addr.sun_family      = AF_UNIX;
  strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

  /* Connect */
  if (connect(clientfd, (const struct sockaddr *)&addr, addrlen) == -1) {
    perror("connect");
    close(clientfd);
    return EXIT_FAILURE;
  }

  /* Send message and time_t to server */
  if (!(argc == 3)) {
    return EXIT_FAILURE;
  }

  snprintf(buffer, BUFFER_SIZE, "%ld|%s", ttime, msg);
  if (write(clientfd, buffer, strlen(buffer)) == -1) {
    perror("write");
    close(clientfd);
    return EXIT_FAILURE;
  }

  shutdown(clientfd, SHUT_WR);

  close(clientfd);
  return 0;
}
