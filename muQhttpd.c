#include "async_log.h"
#include "handle_http.h"
#include "read_cmdline.h"
#include "read_conf.h"

#include <arpa/inet.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int
main(int argc, char* argv[])
{
    /*  1. read command line options */

    struct muQcmdLine* cmd_options;

    if ((cmd_options = read_cmdline(argc, argv)) == NULL) {
        fprintf(stderr, "** Invalid command line options\n");
        fprintf(stderr,
                "Usage:\n  muqhttpd [--wwwdir www-dir] [--confdir conf-dir] "
                "[--logdir log-dir]\n");
        exit(EXIT_FAILURE);
    }

    /*  2. check if all three directories exist and are writeable */

    // FIXME: assume they exist and are writeable

    /*  3. read configuration */

    struct muQconf* conf;

    if ((conf = read_conf(cmd_options->confdir)) == NULL) {
        fprintf(stderr, "Error in configuration file\n");
        exit(EXIT_FAILURE);
    }

    /*  4. init log facilty */

    init_logger(cmd_options->logdir, conf->log_level);

    /*  5. open socket & bind and listen addr */

    int srv_sockfd;
    struct sockaddr_in srv_sockaddr;

    if ((srv_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        alog(LOG_LEVEL_FATEL, "Couldn't open socket");
        exit(EXIT_FAILURE);
    }

    memset(&srv_sockaddr, 0, sizeof(srv_sockaddr));
    srv_sockaddr.sin_family = AF_INET;
    srv_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_sockaddr.sin_port = htons(conf->v4port);

    char ipv4_pbuf[INET_ADDRSTRLEN];
    memset(ipv4_pbuf, 0, sizeof(ipv4_pbuf));
    inet_ntop(AF_INET, (void*)&srv_sockaddr.sin_addr, ipv4_pbuf,
              INET_ADDRSTRLEN);

    if (bind(srv_sockfd, (struct sockaddr*)&srv_sockaddr,
             sizeof(srv_sockaddr)) == -1) {
        alog(LOG_LEVEL_FATEL, "Couldn't bind socket to IPv4 addr %s:%ld",
             ipv4_pbuf, (long)conf->v4port);
        exit(EXIT_FAILURE);
    }

    if (listen(srv_sockfd, 32) == -1) {
        alog(LOG_LEVEL_FATEL, "Couldn't listen IPv4 addr %s:%ld", ipv4_pbuf,
             (long)conf->v4port);
        exit(EXIT_FAILURE);
    }

    alog(LOG_LEVEL_INFO, "muQhttpd running on %s:%ld, muQ, muQ...", ipv4_pbuf,
         (long)conf->v4port);

    /*  6. accept client conncetion */

    int cl_sockfd;
    struct sockaddr_in cl_addr;
    socklen_t cl_add_len = sizeof(cl_addr);

    if (conf->max_thread_num != 0)
        max_http_handler_threads_num(conf->max_thread_num);

    for (;;) {
        cl_sockfd = accept(srv_sockfd, (struct sockaddr*)&cl_addr, &cl_add_len);

        if (cl_sockfd == -1) {
            alog(LOG_LEVEL_ERROR,
                 "Syscall accept return invalid client_sockfd");
            continue;
        }

        inet_ntop(AF_INET, (void*)&cl_addr.sin_addr, ipv4_pbuf,
                  INET_ADDRSTRLEN);
        alog(LOG_LEVEL_INFO, "Receive connection from %s:%ld", ipv4_pbuf,
             (long)ntohs(cl_addr.sin_port));
        handle_http(cl_sockfd);
    }

    return 0;
}
