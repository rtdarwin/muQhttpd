#ifndef READ_CMDLINE_H
#define READ_CMDLINE_H

struct muQcmdLine
{
    char* wwwdir;
    char* confdir;
    char* logdir;
};

struct muQcmdLine* read_cmdline(int argc, char* argv[]);

#endif // READ_CMDLINE_H
