
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/poll.h>
#include <fcntl.h>

char buffer[4096];

int main(
  int argc,
  char **argv){
  struct pollfd pfd;
  int n;

  int fd = open("/dev/burst", O_RDONLY | O_NONBLOCK);
  if(fd < 1) {
	  printf("/dev/burst open error\n");
	  exit(0);
  }
//  fcntl(fd, F_SETFL, fcntl(0,F_GETFL) | O_NONBLOCK);
  pfd.fd = fd;
  pfd.events = POLLIN;

  while (1) {
    n=read(fd, buffer, 4096);
    if (n >= 0)
      write(0, buffer, n);
    n = poll(&pfd, 1, -1);
    if (n < 0)
	  break;
  }
  perror( n<0 ? "stdin" : "stdout");
  exit(1);
}
