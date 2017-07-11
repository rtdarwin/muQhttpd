#include "read_conf.h"
#include "async_log.h"
#include <stdlib.h>

static struct muQconf conf;

struct muQconf*
read_conf(char* conf_dir)
{
    // FIXME: now return a fake muQconf

    conf.v4port = 4000;
    conf.log_level = LOG_LEVEL_DEBUG;
    conf.max_thread_num = 1024;

    return &conf;
}
