#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../headers/reminder.h"

volatile sig_atomic_t keep_running = 1;

void signal_handler(int sig)
{
  if (sig == SIGTERM) {
    keep_running = 0;
    syslog(LOG_INFO, "Daemon recieved SIGTERM, shutting down!");
  }
}

int main(void)
{
  struct    sigaction   siga;
  struct    sockaddr_un addr;

  int       serverfd, clientfd;
  
  char      buffer[BUFFER_SIZE];
  socklen_t addrlen = sizeof(struct sockaddr_un);
  ssize_t   r;

  /* Server socket */
  umask(0111);
  if ((serverfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    return EXIT_FAILURE;
  }

  /* Unlink old socket if exists */
  unlink(SOCKET_PATH);
  
  /* Address */
  memset(&addr, 0, addrlen);
  addr.sun_family      = AF_UNIX;
  strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

  /* Bind the server socket */
  if (bind(serverfd, (struct sockaddr *)&addr, addrlen) == -1) {
    perror("bind");
    close(serverfd);
    return EXIT_FAILURE;
  }

  /* Listen incoming conns */
  if (listen(serverfd, BACKLOG) == -1) {
    perror("listen");
    close(serverfd);
    return EXIT_FAILURE;
  }

  printf("Server started: %s\n", SOCKET_PATH);

  /* Signals */
  siga.sa_handler = signal_handler;
  siga.sa_flags   = 0;

  sigemptyset(&siga.sa_mask);

  openlog("reminderd", LOG_PID, LOG_DAEMON);

  if (sigaction(SIGTERM, &siga, NULL) == -1) {
    syslog(LOG_ERR, "Failed to set signal handler!");
    closelog();
    return EXIT_FAILURE;
  }

  syslog(LOG_INFO, "Daemon started :)");

  /* Running daemon */
  while (keep_running) {
      clientfd = accept(serverfd, NULL, NULL);
      if (clientfd == -1) {
        perror("accept");
        continue;
      } 
      /* Read data from the client */
      // TO-DO: accept string and time
      r = read(clientfd, buffer, BUFFER_SIZE - 1);
      if (r > 0) {
        buffer[r] = '\0';
        syslog(LOG_INFO, "Client: %s\n", buffer);
        
        const char *reply = "Hello from server :)";
        if (write(clientfd, reply, strlen(reply)) == -1) {
          perror("write");
          continue;
        }
      }

      close(clientfd);
  }

  syslog(LOG_INFO, "Daemon shutting down");
  close(serverfd);
  unlink(SOCKET_PATH);
  closelog();

  return EXIT_SUCCESS;
}
