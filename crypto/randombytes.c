/* taken from nacl-20110221, from randombytes/devurandom.c, added close-on-exec */
#include <inc/types.h>
#include <inc/stat.h>
#include <inc/fcntl.h>
#include <inc/unistd.h>
#include <inc/lib.h>
#include "randombytes.h"

/* it's really stupid that there isn't a syscall for this */

static int fd = -1;
// TODO:
void randombytes(unsigned char *x,unsigned long long xlen)
{
  for (size_t i = 0; i < xlen; ++i) {
    x[i] = 1;
  }
  return;
  
  int i;

  if (fd == -1) {
    for (;;) {
#ifdef O_CLOEXEC
      fd = open("/dev/urandom",O_RDONLY | O_CLOEXEC);
#else
      fd = open("/dev/urandom",O_RDONLY);
      fcntl(fd,F_SETFD,1);
#endif
      if (fd != -1) break;
      sleep(1);
    }
  }

  while (xlen > 0) {
    if (xlen < 1048576) i = xlen; else i = 1048576;

    i = read(fd,x,i);
    if (i < 1) {
      sleep(1);
      continue;
    }

    x += i;
    xlen -= i;
  }
}
