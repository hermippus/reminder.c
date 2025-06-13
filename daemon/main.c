#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/syslog.h>
#include <unistd.h>
#include <syslog.h>
 
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
  struct sigaction siga;

  siga.sa_handler = signal_handler;
  siga.sa_flags   = 0;

  sigemptyset(&siga.sa_mask);

  if (sigaction(SIGTERM, &siga, NULL) == -1) {
    syslog(LOG_ERR, "Failed to set signal handler!");
    closelog();
    return EXIT_FAILURE;
  }

  openlog("reminderd", LOG_PID, LOG_DAEMON);
  syslog(LOG_INFO, "Daemon started!");

  while (keep_running) {
    time_t now = time(NULL);
    struct tm *time = localtime(&now);

    syslog(LOG_INFO, "Daemon is running: %02d:%02d:%02d",
           time->tm_hour, time->tm_min, time->tm_sec);
    sleep(10);
  }

  syslog(LOG_INFO, "Daemon finished! Goodbye..");
  closelog();
  return 0;
}
