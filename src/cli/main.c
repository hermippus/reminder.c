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

int main(int argc, char **argv)
{
  int       clientfd;
  struct    sockaddr_un addr;
  char      buffer[BUFFER_SIZE];
  socklen_t addrlen = sizeof(struct sockaddr_un);
  ssize_t   r, w;

  /* Arguments */
  if (argc < 3) {
    usage();
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

  /* Write to server */
  snprintf(buffer, BUFFER_SIZE, "%s", argv[1]);
  w = write(clientfd, buffer, strlen(buffer));
  if (w == -1) {
    perror("write");
    close(clientfd);
    return EXIT_FAILURE;
  }

  shutdown(clientfd, SHUT_WR);

  /* Read from server*/ 
  r = read(clientfd, buffer, BUFFER_SIZE - 1);
  if (r > 0) {
    buffer[r] = '\0';
    printf("Server: %s\n", buffer);
  }

  close(clientfd);
  return 0;
}
