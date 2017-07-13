#include "async_log.h"
#include "handle_connection.h"
#include "read_conf.h"

#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>

int
main(int argc, char* argv[])
{
    signal(SIGPIPE, SIG_IGN);

    /*  1. read command line options */

    char confdir[512] = { 0 };
    strcpy(confdir, "./conf"); // default

    if (argc != 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help")) {
            fprintf(stderr, "Usage:\n  muqhttpd --confdir|-c conf-dir] ");
            exit(EXIT_FAILURE);
        }

        if (argc == 3 &&
            (strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "--confdir") == 0)) {
            strncpy(confdir, argv[2], 511);
        } else {
            fprintf(stderr, "Usage:\n  muqhttpd --confdir|-c conf-dir] ");
            exit(EXIT_FAILURE);
        }
    }

    // FIXME: assume they exist and are writeable

    /*  3. read configuration */

    struct muQconf conf;

    if ((read_conf_file(confdir, &conf)) == -1) {
        fprintf(stderr, "Error in configuration file\n");
        exit(EXIT_FAILURE);
    }

    /*  4. init log facilty */

    init_logger(conf.logdir, conf.log_level);

    /*  5. open socket & bind and listen addr */

    int srv_sockfd;
    struct sockaddr_in srv_sockaddr;

    if ((srv_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Couldn't open socket\n");
        exit(EXIT_FAILURE);
    }

    int on = 1;
    if ((setsockopt(srv_sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) ==
        -1) {
        fprintf(stderr, "Couldn't use setsockopt to set SO_REUSEADDR\n");
        exit(EXIT_FAILURE);
    }

    memset(&srv_sockaddr, 0, sizeof(srv_sockaddr));
    srv_sockaddr.sin_family = AF_INET;
    srv_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_sockaddr.sin_port = htons(conf.v4port);

    char ipv4_pbuf[INET_ADDRSTRLEN];
    memset(ipv4_pbuf, 0, sizeof(ipv4_pbuf));
    inet_ntop(AF_INET, (void*)&srv_sockaddr.sin_addr, ipv4_pbuf,
              INET_ADDRSTRLEN);

    if (bind(srv_sockfd, (struct sockaddr*)&srv_sockaddr,
             sizeof(srv_sockaddr)) == -1) {
        fprintf(stderr, "Couldn't bind socket to IPv4 addr %s:%ld", ipv4_pbuf,
                (long)conf.v4port);
        exit(EXIT_FAILURE);
    }

    if (listen(srv_sockfd, 32) == -1) {
        fprintf(stderr, "Couldn't listen to IPv4 addr %s:%ld", ipv4_pbuf,
                (long)conf.v4port);
        exit(EXIT_FAILURE);
    }

    alog(LOG_LEVEL_INFO, "muQhttpd running on %s:%ld, muQ, muQ...", ipv4_pbuf,
         (long)conf.v4port);

    /*  6. accept client conncetion */

    int cl_sockfd;
    struct sockaddr_in cl_addr;
    socklen_t cl_add_len = sizeof(cl_addr);
    struct client_info cl_info;

    if (conf.max_thread_num != 0)
        set_max_connection_handler_threads(conf.max_thread_num);
    else
        set_max_connection_handler_threads(1024);

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

        snprintf(cl_info.ipv4addr, INET_ADDRSTRLEN, "%s", ipv4_pbuf);
        cl_info.port = ntohs(cl_addr.sin_port);
        cl_info.sockfd = cl_sockfd;
        handle_connection(&cl_info);
    }

    return 0;
}
