#ifndef ASYNC_LOG_H
#define ASYNC_LOG_H

#define LOG_LEVEL_FATEL 3
#define LOG_LEVEL_ERROR 2
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_DEBUG 0

int init_logger(char* logdir, int log_level);
int set_log_level(int log_level);
int alog(int loglevel, char* fmt, ...);

#endif // ASYNC_LOG_H
