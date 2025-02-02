/* taken from nacl-20110221, from curvecp/open_cwd.c curvecp/open_pipe.c curvecp/open_read.c  curvecp/open_write.c */
#include <inc/types.h>
#include <inc/stat.h>
#include <inc/unistd.h>
#include <inc/fcntl.h>
#include <inc/stdio.h>
#include <inc/lib.h>
#include <inc/fcntl.h>

#include "debug.h"
#include "blocking.h"
#include "open.h"

int open_cwd(void)
{
  return open_read(".");
}

int open_pipe(int *fd)
{
  int i;
  if (pipe(fd) == -1) return -1;
  for (i = 0;i < 2;++i) {
    fcntl(fd[i],F_SETFD,1);
    blocking_disable(fd[i]);
  }
  return 0;
}

int open_read(const char *fn)
{
#ifdef O_CLOEXEC
  return open(fn,O_RDONLY | O_NONBLOCK | O_CLOEXEC);
#else
  dprintf("try open %s\n", fn);
  int fd = open(fn, O_RDONLY);
  if (fd == -1) return -1;
  // fcntl(fd,F_SETFD,1);
  return fd;
#endif
}

int open_write(const char *fn)
{
#ifdef O_CLOEXEC
  return open(fn,O_CREAT | O_WRONLY | O_NONBLOCK | O_CLOEXEC,0644);
#else
  int fd = open(fn,O_CREAT | O_WRONLY | O_NONBLOCK,0644);
  if (fd == -1) return -1;
  fcntl(fd,F_SETFD,1);
  return fd;
#endif
}
