/*
20140120
Jan Mojzis
Public domain.
*/

#include <inc/unistd.h>
#include "inc/socket.h"
#include "writeall.h"
#include "e.h"
#include "byte.h"
#include "purge.h"
#include "packet.h"

#include <inc/stdio.h>

int packet_sendisready(void) {

    return (packet.sendbuf.len > 0);
}


int packet_send(void) {

    struct buf *sendbuf = &packet.sendbuf;
    long long w;

    if (sendbuf->len <= 0) return 1;
    w = write(1, sendbuf->buf, sendbuf->len);
    if (w == -1) {
        if (errno == EINTR) return 1;
        if (errno == EAGAIN) return 1;
        if (errno == EWOULDBLOCK) return 1;
        return 0;
    }
    byte_copy(sendbuf->buf, sendbuf->len - w, sendbuf->buf + w);
    sendbuf->len -= w;
    purge(sendbuf->buf + sendbuf->len, w);
    return 1;
}

int packet_sendall(void) {
    int res = devsocket_send(packet.sendbuf.buf, packet.sendbuf.len);
    if (res != packet.sendbuf.len) {
        printf("error while send packet with len %d\n", res);
    }
    printf("send all packets with len %lld\n", packet.sendbuf.len);
    purge(packet.sendbuf.buf, packet.sendbuf.len);
    packet.sendbuf.len = 0;
    return 1;
}
