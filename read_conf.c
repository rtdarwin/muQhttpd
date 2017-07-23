#include "read_conf.h"
#include "async_log.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <strings.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static char* confdir_;
static struct muQconf conf_;

static void
eat_blank(char** buf_pptr, int max_len)
{
    int cnt = 0;

    while (**buf_pptr == ' ' && cnt++ < max_len) {
        (*buf_pptr)++;
    }
}

int
read_conf_file(char* dir, struct muQconf* conf_ret)
{
    /*  1. Set default value */

    conf_.v4port = 4000;
    conf_.log_level = LOG_LEVEL_DEBUG;
    conf_.max_thread_num = 4096;
    strcpy(conf_.logdir, "./log");
    strcpy(conf_.wwwdir, "./www");
    strcpy(conf_.cgibindir, "./www/cgi-bin");

    /*  2. Open file */

    confdir_ = dir;
    char conf_file[1024];
    sprintf(conf_file, "%s/muqhttpd.conf", confdir_);

    FILE* conf_stream;

    if ((conf_stream = fopen(conf_file, "r")) == NULL) {
        fprintf(stderr, "Couldn't open configuration file %s, %s\n", conf_file,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*  3. Read file line by line */

    char linebuf[1024];
    char* value_begin = NULL;

    while (fgets(linebuf, 1024, conf_stream) != NULL) {
        if (linebuf[0] == '#')
            continue;

        if (strncasecmp(linebuf, "v4port", 6) == 0) {
            value_begin = strchr(linebuf, ':') + 1;
            eat_blank(&value_begin, 1024 - 6);

            conf_.v4port = (short)strtol(value_begin, NULL, 10);

        } else if (strncasecmp(linebuf, "max_threads_num", 15) == 0) {
            value_begin = strchr(value_begin, ':') + 1;
            eat_blank(&value_begin, 1024 - 15);

            conf_.max_thread_num = (int)strtol(value_begin, NULL, 10);

        } else if (strncasecmp(value_begin, "log_level", 8) == 0) {
            value_begin = strchr(value_begin, ':') + 1;
            eat_blank(&value_begin, 1024 - 8);

            if (strncasecmp(value_begin, "DEBUG", 6) == 0) {
                conf_.log_level = LOG_LEVEL_DEBUG;
            } else if (strncasecmp(value_begin, "INFO", 4) == 0) {
                conf_.log_level = LOG_LEVEL_INFO;
            } else if (strncasecmp(value_begin, "WARN", 4) == 0) {
                conf_.log_level = LOG_LEVEL_WARN;
            } else if (strncasecmp(value_begin, "ERROR", 5) == 0) {
                conf_.log_level = LOG_LEVEL_ERROR;
            } else if (strncasecmp(value_begin, "FATEL", 5) == 0) {
                conf_.log_level = LOG_LEVEL_FATEL;
            }

        } else if (strncasecmp(value_begin, "wwwdir", 6) == 0) {
            value_begin = strchr(value_begin, ':');
            eat_blank(&value_begin, 1024 - 6);

        } else if (strncasecmp(value_begin, "cgibindir", 9) == 0) {
            value_begin = strchr(value_begin, ':');
            eat_blank(&value_begin, 1024 - 9);

        } else if (strncasecmp(value_begin, "logdir", 6) == 0) {
            value_begin = strchr(value_begin, ':');
            eat_blank(&value_begin, 1024 - 6);
        }
    }

    *conf_ret = conf_;
    return 0;
}

struct muQconf
get_conf()
{
    return conf_;
}
