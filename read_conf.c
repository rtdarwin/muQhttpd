#include "read_conf.h"
#include "async_log.h"
#include <stdlib.h>
#include <string.h>

static char* confdir_;
static struct muQconf conf_;

int
read_conf_file(char* dir, struct muQconf* conf_ret)
{
    /*  1. Set default value */

    conf_.v4port = 4000;
    conf_.log_level = LOG_LEVEL_DEBUG;
    conf_.max_thread_num = 1024;
    strcpy(conf_.logdir, "./log");
    strcpy(conf_.wwwdir, "./www");
    strcpy(conf_.cgibindir, "./www/cgi-bin");

    /*  2. Read file to override default conf */

    confdir_ = dir;
    // TODO

    *conf_ret = conf_;
    return 0;
}

struct muQconf
get_conf()
{
    return conf_;
}
