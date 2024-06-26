/* taken from nacl-20110221, from curvecp/writeall.c */
#include "writeall.h"
#include "e.h"
#include <inc/poll.h>
#include <inc/unistd.h>

int writeall(int fd, const void *xv, long long xlen) {
  devsocket_send((char *)xv, xlen);
  return 0;
}
