#include "server.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s PORT\n", argv[0]);
    printf("\tPORT - a number which represents the port (1-65535)\n");
    printf("\t\tusually 80 is for web\n");
    return 1;
  }

  /* Separate the process and make as a daemon */
  if (fork() != 0)
    return 1; // failed to fork

  // ignore some shutdown signals:
  // http://stackoverflow.com/questions/18551485/how-to-make-the-process-ignore-some-signallike-sighup-sigabrt-sigabort-sigint-e
  signal(SIGHUP, SIG_IGN); // ignore terminal exits
  signal(SIGCLD, SIG_IGN); // ignore child exits
  int port = atoi(argv[1]);
  int sock;
  struct sockaddr_in sin;
  sock = socket(AF_INET, SOCK_STREAM, 0);

  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(port);

  if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) != 0) {
    log_error("Failed to bind to socket");
    return 1;
  }

  listen(sock, 5);
  log_info("HTTP server listening on port %d", port);
  char *root = get_root();
  int loop_result = loop(sock, root);
  while (loop_result != 1) {
    loop_result = loop(sock, root);
  }
  free(root);
  close(sock);
  return loop_result;
}