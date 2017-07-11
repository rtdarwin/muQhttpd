#include "async_log.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/syscall.h>

#ifdef SYS_gettid
#define gettid() syscall(SYS_gettid)
#else
#define gettid() 0
#endif

int
init_logger(char* logdir, int log_level)
{
}

int
set_log_level(int log_level)
{
}

int
alog(int loglevel, char* fmt, ...)
{
}
