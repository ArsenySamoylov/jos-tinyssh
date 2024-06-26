/*
20140323
Jan Mojzis
Public domain.
*/

#include <inc/poll.h>
#include <inc/unistd.h>
#include <inc/socket.h>
#include "e.h"
#include "inc/lib.h"
#include "getln.h"

static int getch(int fd, char *x) {

    int r;
    struct pollfd p;

    for (;;) {
        r = read(fd, x, 1);
        if (r == -1) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                p.fd = fd;
                p.events = POLLIN | POLLERR;
                poll(&p, 1, -1);
                continue;
            }
        }
        break;
    }
    return r;
}

/*
The function 'getln' reads line from filedescriptor 'fd' into
buffer 'xv' of length 'xmax'. 
*/
int getln(int fd, void *xv, long long xmax) {

    long long xlen;
    int r;
    char ch;
    char *x = (char *)xv;

    if (xmax < 1) { errno = EINVAL; return -1; }
    x[0] = 0;
    if (fd < 0) { errno = EBADF; return -1; }

    xlen = 0;
    for (;;) {
        if (xlen >= xmax - 1) { x[xmax - 1] = 0; errno = ENOMEM; return -1; }
        r = devsocket_recv(&ch, 1);
        if (r == 0) continue;
        if (ch == 0) ch = '\n';
        x[xlen++] = ch;
        if (ch == '\n') break;
        sys_yield();
    }
    x[xlen] = 0;
    return r;
}
