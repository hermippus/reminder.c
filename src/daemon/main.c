#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

#include "../headers/reminder.h"

#define MAX_EVENTS 10

volatile sig_atomic_t keep_running = 1;

Reminder reminders[100];
int reminder_count = 0;

void signal_handler(int sig)
{
  if (sig == SIGTERM) {
    keep_running = 0;
    syslog(LOG_INFO, "Daemon received SIGTERM, shutting down!");
  }
}

int main(void)
{
  struct      sigaction siga;
  struct      sockaddr_un addr;
  int         serverfd, clientfd;
  char        buffer[BUFFER_SIZE];
  socklen_t   addrlen = sizeof(struct sockaddr_un);
  ssize_t     r;
  Reminder    reminder;

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

  /* Signals */
  siga.sa_handler = signal_handler;
  siga.sa_flags   = 0;

  sigemptyset(&siga.sa_mask);

  if (sigaction(SIGTERM, &siga, NULL) == -1) {
    syslog(LOG_ERR, "Failed to set signal handler!");
    closelog();
    return EXIT_FAILURE;
  }

  openlog("reminderd", LOG_PID, LOG_DAEMON);
  syslog(LOG_INFO, "Daemon started :)");

  /* Epoll */
  int epfd = epoll_create1(0);
  if (epfd == -1) {
    perror("epoll_create1");
    return EXIT_FAILURE;
  }

  struct epoll_event ev, events[MAX_EVENTS];
  ev.events = EPOLLIN;
  ev.data.fd = serverfd;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, serverfd, &ev) == -1) {
    perror("epoll_ctl serverfd");
    return EXIT_FAILURE;
  }

  int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
  if (tfd == -1) {
    perror("timerfd_create");
    return EXIT_FAILURE;
  }

  struct itimerspec timer;
  timer.it_interval.tv_sec  = 1;
  timer.it_interval.tv_nsec = 0;
  timer.it_value.tv_sec     = 1;
  timer.it_value.tv_nsec    = 0;

  if (timerfd_settime(tfd, 0, &timer, NULL) == -1) {
    perror("timerfd_settime");
    return EXIT_FAILURE;
  }

  ev.events   = EPOLLIN;
  ev.data.fd  = tfd;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &ev) == -1) {
    perror("epoll_ctl timerfd");
    return EXIT_FAILURE;
  }

  /* Running daemong and accept data */
  while (keep_running) {
    int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
    if (n == -1) {
      if (errno == EINTR) continue;
      perror("epoll_wait");
      break;
    }

    for (int i = 0; i < n; ++i) {
      if (events[i].data.fd == serverfd) {
        clientfd = accept(serverfd, NULL, NULL);
        if (clientfd != -1) {
          memset(buffer, 0, BUFFER_SIZE);
          r = read(clientfd, buffer, BUFFER_SIZE - 1);

          if (r > 0) {
            buffer[r] = '\0';
            char *sep = strchr(buffer, '|');

            if (sep) {
              *sep = '\0';
              reminder.time = (time_t)atol(buffer);
              strncpy(reminder.message, sep + 1, BUFFER_SIZE - 1);
              reminder.message[BUFFER_SIZE - 1] = '\0';

              reminders[reminder_count++] = reminder;
            } else {
              syslog(LOG_WARNING, "Invalid reminder format received");
            }
          }

          close(clientfd);
        }

      } else if (events[i].data.fd == tfd) {
        uint64_t expirations;
        read(tfd, &expirations, sizeof(expirations));

        time_t now = time(NULL);
        for (int j = 0; j < reminder_count; ++j) {
          if (reminders[j].time <= now && reminders[j].time != 0) {
            syslog(LOG_INFO, "Reminder: %s", reminders[j].message);
            reminders[j].time = 0;
          }
        }
      }
    }
  }

  syslog(LOG_INFO, "Daemon shutting down");
  close(tfd);
  close(serverfd);
  unlink(SOCKET_PATH);
  closelog();

  return EXIT_SUCCESS;
}

