#include "global.h"
#include "inc/stdio.h"
#include "log.h"
#include "die.h"

void die_usage(const char *x) {

    log_u1(x);
    global_die(100);
}

void die_fatal_(const char *fn, unsigned long long line, const char *trouble, const char *d, const char *f) {

    cprintf("\n%s:%lld %s\n", fn, line, trouble);
    if (d) {
        if (f) {
            cprintf("in %s:%s\n", d, f);
        } else {
            cprintf("int %s\n", d);
        }
    }
    global_die(111);
}
