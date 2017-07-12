#ifndef ASYNC_LOG_BACKEND_H
#define ASYNC_LOG_BACKEND_H

#include <mqueue.h>

void init_logger_backend(mqd_t msgq, char *logdir);

#endif
