/* taken from nacl-20110221, from curvecp/blocking.c */
#include <inc/fcntl.h>
#include "blocking.h"

void blocking_enable(int fd)
{
  fcntl(fd,F_SETFL,fcntl(fd,F_GETFL,0) & ~O_NONBLOCK);
}

void blocking_disable(int fd)
{
  fcntl(fd,F_SETFL,fcntl(fd,F_GETFL,0) | O_NONBLOCK);
}
