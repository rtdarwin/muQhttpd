#ifndef READ_CONF_H
#define READ_CONF_H

#include "inttypes.h"

struct muQconf
{
    uint16_t v4port;
    int max_thread_num;
    int log_level;
    char wwwdir[512];
    char logdir[512];
    char cgibindir[512];
};

/* Store the conf in param#2 conf, return -1 if conf file contains error */
int read_conf_file(char* conf_dir, struct muQconf* conf);
struct muQconf get_conf();

#endif // READ_CONF_H
