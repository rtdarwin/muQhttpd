#include "read_cmdline.h"
#include <stdlib.h>

static struct muQcmdLine options;

struct muQcmdLine*
read_cmdline(int argc, char* argv[])
{
    // FIXME: new return a fake muQcmdLine

    options.wwwdir = "./www";
    options.confdir = "./conf";
    options.logdir = "./log";

    return &options;
}
