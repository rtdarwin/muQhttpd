#include "async_log_backend.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define min(m, n) ((m) < (n) ? (m) : (n))
#define max(m, n) ((m) > (n) ? (m) : (n))

static char* logdir_;
static mqd_t msgq_;
static int logfd_;

static void*
log_backend_thread(void* unused)
{
    /*  1. Open log file to write */

    char log_name[256];
    snprintf(log_name, 256, "%s/muQhttpd.log", logdir_);

    int o_flags = O_WRONLY | O_APPEND | O_CREAT;
    mode_t perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; /* rw-r--r-- */
    logfd_ = open(log_name, o_flags, perms);

    if (logfd_ == -1) {
        fprintf(stderr, "Error when  when initing async_log module"
                        ": couldn't open file to log"
                        " - %s:%d\n",
                __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

    /*  2. Prepare buf to receive message */

    size_t buf_len;

    struct mq_attr msgq_attr;
    mq_getattr(msgq_, &msgq_attr);
    buf_len = max(1024, msgq_attr.mq_msgsize);

    char buf[buf_len];
    ssize_t nrecv;
    ssize_t nwrite;

    /*  3. Read log from msgq, then write to log file */

    for (;;) {
        // if receive message error
        nrecv = mq_receive(msgq_, buf, buf_len, NULL);
        if (nrecv == -1) {
            fprintf(stderr, "Error when reading message from msgq"
                            " - %s:%d\n",
                    __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }

        // if write file error
        nwrite = write(logfd_, buf, nrecv);
        if (nwrite == -1) {
            fprintf(stderr, "Error when write log to file"
                            " - %s:%d\n",
                    __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }

        // if receive fatel level log
        buf[nrecv] = '\0';
        if (strstr(buf, "FATEL")) {
            exit(EXIT_FAILURE);
        }
    }
    return NULL;
}

void
init_logger_backend(mqd_t msgq, char* logdir)
{
    logdir_ = logdir;
    msgq_ = msgq;

    /*  1. Start a log thread  */

    pthread_t logthread;
    if (pthread_create(&logthread, NULL, &log_backend_thread, NULL) > 0) {
        fprintf(stderr, "Error when initing async_log module"
                        ": couldn't create async log thread"
                        " - %s:%d\n",
                __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
