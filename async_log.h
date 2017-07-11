#ifndef ASYNC_LOG_H
#define ASYNC_LOG_H

#define LOG_LEVEL_FATEL 4
#define LOG_LEVEL_ERROR 3
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_DEBUG 0

#define MAX_LOG_LENGTH 1024

void init_logger(char* logdir, int log_level);
void set_log_level(int log_level);

/* time                     tid   level user message
 * 20170707 11:11:11.125737 23612 INFO Receive request from ...  */
void alog(int log_level, char* fmt, ...);

#endif // ASYNC_LOG_H
