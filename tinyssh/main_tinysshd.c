/*
20140107
Jan Mojzis
Public domain.
*/
#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/stat.h>
#include <inc/unistd.h>
#include <inc/signal.h>
#include <inc/poll.h>
#include <inc/socket.h>
#include <inc/fcntl.h>
#include <inc/lib.h>

#include "debug.h"
#include "blocking.h"
#include "ssh.h"
#include "purge.h"
#include "open.h"
#include "load.h"
#include "e.h"
#include "byte.h"
#include "buf.h"
#include "packet.h"
#include "channel.h"
#include "log.h"
#include "sshcrypto.h"
#include "subprocess.h"
#include "global.h"
#include "connectioninfo.h"
#include "die.h"
#include "str.h"
#include "main.h"
#include "env.h"

static unsigned int cryptotypeselected = sshcrypto_TYPENEWCRYPTO | sshcrypto_TYPEPQCRYPTO;
static int flagverbose = 1;
static int fdwd;
static int flaglogger = 0;
static const char *customcmd = 0;
static int flagnoneauth = 0;

static struct buf b1 = {global_bspace1, 0, sizeof global_bspace1};
static struct buf b2 = {global_bspace2, 0, sizeof global_bspace2};

static void timeout(int x) {
    errno = x = ETIMEDOUT;
    die_fatal("closing connection", 0, 0);
}

static int selfpipe[2] = { -1, -1 };

static void trigger(int x) {
    errno = 0;
    x = write(selfpipe[1], "", 1);
}

char *environ_array[] = {"PATH=/;/bin", NULL}; 

int main_tinysshd(int argc, char **argv, const char *binaryname) {
    environ = environ_array;

    char *x;
    const char *keydir = 0;
    long long i;
    struct pollfd p[6];
    struct pollfd *q;
    struct pollfd *watch0;
    struct pollfd *watch1;
    struct pollfd *watchtochild;
    struct pollfd *watchfromchild;
    struct pollfd *watchselfpipe;
    int exitsignal, exitcode;
    long long binarynamelen = str_len(binaryname);
    const char *usage;

    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, timeout);

    dprintf("set signals\n");
    close(0);
    close(1);
    int sock_fd0 = opensock();
    assert(sock_fd0 == 0);
    int sock_fd1 = sock_fd0 + 1;;
    dup(sock_fd0, sock_fd1);
    dprintf("open socket\n");

    if (str_equaln(binaryname, binarynamelen, "tinysshnoneauthd")) {
        usage = "usage: tinysshnoneauthd [options] keydir";
    }
    else {
        usage = "usage: tinysshd [options] keydir";
    }

    if (argc < 2) die_usage(usage);
    if (!argv[0]) die_usage(usage);
    for (;;) {
        if (!argv[1]) break;
        if (argv[1][0] != '-') break;
        x = *++argv;
        if (x[0] == '-' && x[1] == 0) break;
        if (x[0] == '-' && x[1] == '-' && x[2] == 0) break;
        while (*++x) {
            if (*x == 'q') { flagverbose = 0; continue; }
            if (*x == 'Q') { flagverbose = 1; continue; }
            if (*x == 'v') { ++flagverbose; if (flagverbose >= 4) flagverbose = 4; continue; }
            if (*x == 'o') { cryptotypeselected |= sshcrypto_TYPEOLDCRYPTO; continue; }
            if (*x == 'O') { cryptotypeselected &= ~sshcrypto_TYPEOLDCRYPTO; continue; }
            if (*x == 's') { cryptotypeselected |= sshcrypto_TYPENEWCRYPTO; continue; }
            if (*x == 'S') { cryptotypeselected &= ~sshcrypto_TYPENEWCRYPTO; continue; }
            if (*x == 'p') { cryptotypeselected |= sshcrypto_TYPEPQCRYPTO; continue; }
            if (*x == 'P') { cryptotypeselected &= ~sshcrypto_TYPEPQCRYPTO; continue; }
            if (*x == 'l') { flaglogger = 1; continue; }
            if (*x == 'L') { flaglogger = 0; continue; }
            if (*x == 'x') {
                if (x[1]) { channel_subsystem_add(x + 1); break; }
                if (argv[1]) { channel_subsystem_add(*++argv); break; }
            }
            if (*x == 'e') {
                if (x[1]) { customcmd = x + 1; break; }
                if (argv[1]) { customcmd = *++argv; break; }
            }

            die_usage(usage);
        }
    }
    keydir = *++argv; if (!keydir) die_usage(usage);

    if (str_equaln(binaryname, binarynamelen, "tinysshnoneauthd")) {
        if (!customcmd) die_fatal("rejecting to run without -e customprogram", 0, 0);
        if (geteuid() == 0) die_fatal("rejecting to run under UID=0", 0, 0);
        flagnoneauth = 1;
    }
    global_init();

    dprintf("blocking disable\n");

    /* get server longterm keys */

    for (i = 0; sshcrypto_keys[i].name; ++i) sshcrypto_keys[i].sign_flagserver |= sshcrypto_keys[i].cryptotype & cryptotypeselected;
    for (i = 0; sshcrypto_keys[i].name; ++i) sshcrypto_keys[i].sign_flagclient |= sshcrypto_keys[i].cryptotype & cryptotypeselected;
    for (i = 0; sshcrypto_kexs[i].name; ++i) sshcrypto_kexs[i].flagenabled |= sshcrypto_kexs[i].cryptotype & cryptotypeselected;
    for (i = 0; sshcrypto_ciphers[i].name; ++i) sshcrypto_ciphers[i].flagenabled |= sshcrypto_ciphers[i].cryptotype & cryptotypeselected;

    char old_cwd[MAXPATHLEN];
    get_cwd(old_cwd, MAXPATHLEN);
    set_cwd(keydir);
    /* read public keys */
    for (i = 0; sshcrypto_keys[i].name; ++i) {
        if (!sshcrypto_keys[i].sign_flagserver) continue;
        if (load(sshcrypto_keys[i].sign_publickeyfilename, sshcrypto_keys[i].sign_publickey, sshcrypto_keys[i].sign_publickeybytes) == -1) {
            sshcrypto_keys[i].sign_flagserver = 0;
            if (errno == ENOENT) continue;
            die_fatal("unable to read public key from file", keydir, sshcrypto_keys[i].sign_publickeyfilename);
        }
    }
    set_cwd(old_cwd);


    /* set timeout */
    alarm(60);

    /* send and receive hello */
    if (!packet_hello_send()) die_fatal("unable to send hello-string", 0, 0);
    if (!packet_hello_receive()) die_fatal("unable to receive hello-string", 0, 0);

    /* send and receive kex */
    if (!packet_kex_send()) die_fatal("unable to send kex-message", 0, 0);
    if (!packet_kex_receive()) die_fatal("unable to receive kex-message", 0, 0);

    dprintf("\nsuccess send and recieve kex\n");

rekeying:
    /* rekeying */
    alarm(60);
    if (packet.flagrekeying == 1) {
        buf_purge(&packet.kexrecv);
        buf_put(&packet.kexrecv, b1.buf, b1.len);
        if (!packet_kex_send()) die_fatal("unable to send kex-message", 0, 0);
    }

    /* send and receive kexdh */
    if (!packet_kexdh(keydir, &b1, &b2)) die_fatal("unable to process kexdh", 0, 0);
    
    if (packet.flagkeys) log_d1("rekeying: done");
    packet.flagkeys = 1;

    /* note: communication is encrypted */

    /* authentication + authorization */
    if (packet.flagauthorized == 0) {
        dprintf("try user  authentication and authorization\n");
        if (!packet_auth(&b1, &b2, flagnoneauth)) die_fatal("authentication failed", 0, 0);
        packet.flagauthorized = 1;
    }

    /* note: user is authenticated and authorized */
    alarm(3600);
    dprintf("user is authenticated and authorized\n");

    /* main loop */
    for (;;) {
        if (channel_iseof())
            if (!packet.sendbuf.len)
                if (packet.flagchanneleofreceived)
                    break;

        watch0 = watch1 = 0;
        watchtochild = watchfromchild = 0;
        watchselfpipe = 0;

        q = p;

        if (packet_sendisready()) { watch1 = q; q->fd = sock_fd1; q->events = POLLOUT; ++q; }
        if (packet_recvisready()) { watch0 = q; q->fd = sock_fd0; q->events = POLLIN;  ++q; }

        if (channel_writeisready()) { watchtochild = q; q->fd = channel_getfd0(); q->events = POLLOUT; ++q; }
        if (channel_readisready() && packet_putisready()) { watchfromchild = q; q->fd = channel_getfd1(); q->events = POLLIN; ++q; }

        if (selfpipe[0] != -1) { watchselfpipe = q; q->fd = selfpipe[0]; q->events = POLLIN; ++q; }

        if (poll(p, q - p, 60000) < 0) {
            watch0 = watch1 = 0;
            watchtochild = watchfromchild = 0;
            watchselfpipe = 0;
        }
        else {
            if (watch0) if (!watch0->revents) watch0 = 0;
            if (watch1) if (!watch1->revents) watch1 = 0;
            if (watchfromchild) if (!watchfromchild->revents) watchfromchild = 0;
            if (watchtochild) if (!watchtochild->revents) watchtochild = 0;
            if (watchselfpipe) if (!watchselfpipe->revents) watchselfpipe = 0;
        }

        if (watchtochild) {

            /* write data to child */
            if (!channel_write()) die_fatal("unable to write data to child", 0, 0);
        }

        /* read data from child */
        if (watchfromchild) packet_channel_send_data(&b2);

        /* check child */
        if (channel_iseof()) {
            if (selfpipe[0] == -1) if (open_pipe(selfpipe) == -1) die_fatal("unable to open pipe", 0, 0);
            signal(SIGCHLD, trigger);
            if (channel_waitnohang(&exitsignal, &exitcode)) {
                packet_channel_send_eof(&b2);
                if (!packet_channel_send_close(&b2, exitsignal, exitcode)) die_fatal("unable to close channel", 0, 0);
            }
        }

        /* send data to network */
        if (watch1) if (!packet_send()) die_fatal("unable to send data to network", 0, 0);

        /* receive data from network */
        if (watch0) {
            alarm(3600); /* refresh timeout */
            if (!packet_recv()) {
                if (channel_iseof()) break; /* XXX */
                die_fatal("unable to receive data from network", 0, 0);
            }
        }

        /* process packets */
        for (;;) {

            if (!packet_get(&b1, 0)) {
                if (!errno) break;
                die_fatal("unable to get packets from network", 0, 0);
            }
            if (b1.len < 1) break; /* XXX */

            switch (b1.buf[0]) {
                case SSH_MSG_CHANNEL_OPEN:
                    dprintf("SSH_MSG_CHANNEL_OPEN\n");
                    if (!packet_channel_open(&b1, &b2)) die_fatal("unable to open channel", 0, 0);
                    break;
                case SSH_MSG_CHANNEL_REQUEST:
                    dprintf("SSH_MSG_CHANNEL_REQUEST\n");
                    if (!packet_channel_request(&b1, &b2, customcmd)) die_fatal("unable to handle channel-request", 0, 0);
                    break;
                case SSH_MSG_CHANNEL_DATA:
                    dprintf("SSH_MSG_CHANNEL_DATA\n");
                    if (!packet_channel_recv_data(&b1)) die_fatal("unable to handle channel-data", 0, 0);
                    break;
                case SSH_MSG_CHANNEL_EXTENDED_DATA:
                    dprintf("SSH_MSG_CHANNEL_EXTENDED_DATA\n");
                    if (!packet_channel_recv_extendeddata(&b1)) die_fatal("unable to handle channel-extended-data", 0, 0);
                    break;
                case SSH_MSG_CHANNEL_WINDOW_ADJUST:
                    if (!packet_channel_recv_windowadjust(&b1)) die_fatal("unable to handle channel-window-adjust", 0, 0);
                    break;
                case SSH_MSG_CHANNEL_EOF:
                    dprintf("SSH_MSG_CHANNEL_EOF\n");
                    if (!packet_channel_recv_eof(&b1)) die_fatal("unable to handle channel-eof", 0, 0);
                    break;
                case SSH_MSG_CHANNEL_CLOSE:
                    dprintf("SSH_MSG_CHANNEL_CLOSE\n");
                    if (!packet_channel_recv_close(&b1)) die_fatal("unable to handle channel-close", 0, 0);
                    break;
                case SSH_MSG_KEXINIT:
                    goto rekeying;
                default:
                    if (!packet_unimplemented(&b1)) die_fatal("unable to send SSH_MSG_UNIMPLEMENTED message", 0, 0);
            }
        }
    }

    log_i1("finished");
    global_die(0); return 111;
}
