/*
20140117
Jan Mojzis
Public domain.
*/

#include <inc/types.h>
#include <inc/wait.h>
#include <inc/unistd.h>
#include <inc/stdio.h>
#include <inc/lib.h>

#include "debug.h"
#include "load.h"
#include "log.h"
#include "open.h"
#include "writeall.h"
#include "purge.h"
#include "global.h"
#include "bug.h"
#include "e.h"
#include "purge.h"
#include "readall.h"
#include "blocking.h"
#include "sshcrypto.h"
#include "subprocess.h"

/*
The 'subprocess_sign' function reads secret-key from 'keydir' and
sings the data 'x' of length 'xlen' and returns it in 'y'.
Caller is expecting 'y' of length 'ylen'. Signing is done 
in a different process, so secret-key is in a separate
memory space than rest of the program.
*/
int subprocess_sign(unsigned char *y, long long ylen, const char *keydir, unsigned char *x, long long xlen) {


    if (ylen != sshcrypto_sign_bytes) bug_inval();
    if (xlen != sshcrypto_hash_bytes) bug_inval();
    if (!y || !keydir || !x) bug_inval();

    unsigned char sk[sshcrypto_sign_SECRETKEYMAX];
    unsigned char sm[sshcrypto_sign_MAX + sshcrypto_hash_MAX];
    unsigned long long smlen;

    if (load(sshcrypto_sign_secretkeyfilename, sk, sshcrypto_sign_secretkeybytes) == -1) {
        dprintf("sign: unable to load secret-key from file %s%s%s", keydir, "/", sshcrypto_sign_secretkeyfilename);
        purge(sk, sizeof sk);
        global_die(111);
    }
    if (sshcrypto_sign(sm, &smlen, x, sshcrypto_hash_bytes, sk) != 0) { 
        dprintf("sign: unable to sign using secret-key from file %s%s%s", keydir, "/", sshcrypto_sign_secretkeyfilename);
        purge(sk, sizeof sk);
        global_die(111);
    }
    memcpy(y, sm, sshcrypto_sign_bytes);
    purge(sk, sizeof sk);
        
    return  0;
}
