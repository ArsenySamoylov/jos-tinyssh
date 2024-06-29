#pragma once


#ifdef debug_tinyssh
#define dprintf(...) cprintf(__VA_ARGS__)
#else
#define dprintf(...)
#endif
