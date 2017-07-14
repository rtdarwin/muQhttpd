#include "handle_cgi.h"
#include "async_log.h"
#include "read_conf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wait.h>

#define SERVER_SOFTWARE_ENV "SERVER_SOFTWARE=muQhttpd/0.1"
#define GATEWAY_INTERFACE_ENV "GATEWAY_INTERFACE=CGI/1.1"
#define SERVER_PROTOCOL_ENV "SERVER_PROTOCOL=HTTP/1.1"

int
is_cgi_path(char* path)
{
    char cgi_prefix[512] = { 0 };
    snprintf(cgi_prefix, 512, "%s/", get_conf().cgibindir);

    return (strncmp(cgi_prefix, path, strlen(cgi_prefix)) == 0);
}

int
exec_cgi(char* localpath, char* method, char* qstring, int sockfd)
{
    char scriptpath[512] = { 0 };

    /*  1. Split script-path and path-info(if any) */

    /*  eg:

        ./www/cgi-bin/printenv/addtional/path/info
                     ^        ^
                     last_slash
                              pathinfo
         -------------------- ---------------------
         scriptpath           pathinfo
    */
    char* last_slash = localpath + strlen(get_conf().cgibindir);
    char* pathinfo = strchr(last_slash + 1, '/');
    if (pathinfo == NULL) {
        strcpy(scriptpath, localpath);
    } else {
        int scriptpath_len = pathinfo - localpath;
        memcpy(scriptpath, localpath, scriptpath_len);
    }

    alog(LOG_LEVEL_INFO, "CGI script path: %s", scriptpath);

    /*  2. Check file exist and executable */

    if (access(scriptpath, F_OK | X_OK) != 0) {
        return -1;
    }

    /*  3. fork & exec CGI script */

    pid_t child_pid;
    int child_status;

    switch (child_pid = fork()) {
        case -1:
            alog(LOG_LEVEL_ERROR,
                 "exec_cgi: Couldn't 'fork' to create child process - %s:%ld",
                 __FILE__, __LINE__);
            return -1;
        case 0: // child process
            if (dup2(sockfd, STDIN_FILENO) != STDIN_FILENO ||
                dup2(sockfd, STDOUT_FILENO) != STDOUT_FILENO) {
                _exit(EXIT_FAILURE);
            } else {

                char method_env[32];
                char pathinfo_env[512];
                char qstring_env[1024];
                snprintf(method_env, 32, "REQUEST_METHOD=%s", method);
                snprintf(pathinfo_env, 512, "PATH_INFO=%s", pathinfo);
                snprintf(qstring_env, 1024, "QUERY_STRING=%s", qstring);

                char* envp[7] = { SERVER_SOFTWARE_ENV,
                                  SERVER_PROTOCOL_ENV,
                                  GATEWAY_INTERFACE_ENV,
                                  method_env,
                                  pathinfo_env,
                                  qstring_env,
                                  NULL };
                execle(scriptpath, scriptpath, (char*)NULL, envp);

                _exit(EXIT_FAILURE); // if execle return;
            }
        default: // parent process
            waitpid(child_pid, &child_status, 0);
            if (WIFEXITED(child_status) && WEXITSTATUS(child_status) == 0) {
                alog(LOG_LEVEL_INFO, "Success running CGI script '%s'",
                     scriptpath);
            } else {
                alog(LOG_LEVEL_WARN,
                     "Error encountered when running CGI script '%s'",
                     scriptpath);
            }
            return 0;
    }

    // We will never reach here
    return 0;
}
