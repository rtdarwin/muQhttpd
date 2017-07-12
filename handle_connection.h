#ifndef HANDLE_CONNECTION_H
#define HANDLE_CONNECTION_H

#include <arpa/inet.h>
#include <inttypes.h>

struct client_info
{
    char ipv4addr[INET_ADDRSTRLEN];
    uint16_t port;
    int sockfd;
};

void set_max_connection_handler_threads(int n);
void handle_connection(struct client_info* info);

#endif // HANDLE_CONNECTION_H
