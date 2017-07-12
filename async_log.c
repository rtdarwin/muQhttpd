#include "async_log.h"
#include "async_log_backend.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <mqueue.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

// Preprocessor directives below are used to make gettid() avaiable
// Maybe it's not elegant and not portable, but through this we can
// get the same tid number as command `htop`

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/syscall.h>

#ifdef SYS_gettid
#define gettid() syscall(SYS_gettid)
#else
#define gettid() 0
#endif

static int min_log_level_;
char* logdir_;
mqd_t msgq_;

/* 20170707 11:11:11.125737
 *     17           1  6
 * 24 characters totally
 */
static void
get_curr_time(char* buf)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    struct tm broken_time;
    gmtime_r(&tv.tv_sec, &broken_time);

    strftime(buf, 18, "%Y%m%d %H:%M:%S", &broken_time);
    snprintf(buf + 17, 1 + 6 + 1, ".%.6ld", tv.tv_usec);
}

static void
write_log(char* buf, size_t len)
{
    mq_send(msgq_, buf, len, 0);

    // for debug use when developing
    write(STDOUT_FILENO, buf, len);
}

void
init_logger(char* logdir, int log_level)
{
    logdir_ = logdir;

    int o_flags = O_CREAT | O_EXCL | O_NONBLOCK;
    mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    msgq_ = mq_open("/muQhttpd_log", o_flags, perms, NULL);
    if (msgq_ == -1) {
        fprintf(stderr, "Error when initing async_log module\n");
        exit(EXIT_FAILURE);
    }

    min_log_level_ = log_level;
}

void
change_log_level(int log_level)
{
    min_log_level_ = log_level;
}

void
alog(int log_level, char* fmt, ...)
{
    if (log_level < min_log_level_)
        return;

    char* level_str = "DEBUG";
    switch (log_level) {
        case LOG_LEVEL_DEBUG:
            level_str = "DEBUG";
            break;

        case LOG_LEVEL_INFO:
            level_str = "INFO";
            break;

        case LOG_LEVEL_WARN:
            level_str = "WARN";
            break;

        case LOG_LEVEL_ERROR:
            level_str = "ERROR";
            break;

        case LOG_LEVEL_FATEL:
            level_str = "FATEL";
            break;
    }

    // '__thread' ensure every thread have its own logbuf
    // to avoid race condition
    static __thread char logbuf[MAX_LOG_LENGTH];
    int tot_written = 0;

    /*  1. time  */

    get_curr_time(logbuf);
    tot_written += 24;

    /*  2. threadid(linux-specific) and loglevel */

    tot_written += snprintf(logbuf + 24, MAX_LOG_LENGTH - 24, " %ld %s ",
                            (long)gettid(), level_str);

    /*  3. user message */

    va_list arg_list;
    va_start(arg_list, fmt);
    tot_written +=
        vsnprintf(logbuf + tot_written, MAX_LOG_LENGTH - 24 - tot_written + 1,
                  fmt, arg_list);
    va_end(arg_list);

    /*  4. line feed
     *
     *     Here we don't add a terminating '\0', because variable
     *     'tot_written' can determine its boundary.
     */

    logbuf[tot_written++] = '\n';

    write_log(logbuf, tot_written);
}
