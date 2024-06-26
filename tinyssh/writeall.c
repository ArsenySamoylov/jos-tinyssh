/* taken from nacl-20110221, from curvecp/writeall.c */
#include <poll.h>
#include <unistd.h>
#include "e.h"
#include "inc/assert.h"
#include "writeall.h"

int writeall(int fd,const void *xv,long long xlen)
{
  char *x = (char *)xv;
  long long w;
  while (xlen > 0) {
    w = xlen;
    if (w > 1048576) w = 1048576;
    w = write(fd,x,w);
    if (w < 0) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
        struct pollfd p;
        p.fd = fd;
        p.events = POLLOUT | POLLERR;
        panic("//TODO: %s:%s\n", __FILE_NAME__, __FUNCTION__);
        poll(&p,1,-1);
        continue;
      }
      return -1;
    }
    x += w;
    xlen -= w;
  }
  return 0;
}
