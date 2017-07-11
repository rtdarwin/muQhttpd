#ifndef READ_CONF_H
#define READ_CONF_H

#include "inttypes.h"
#include <sys/types.h>

struct muQconf
{
    uint16_t v4port;
    size_t max_thread_num;
    int log_level;
};

struct muQconf* read_conf(char* conf_dir);

#endif // READ_CONF_H
