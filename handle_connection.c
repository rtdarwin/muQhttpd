#include "handle_connection.h"
#include "async_log.h"
#include "handle_http.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>

static sem_t tot_threads_avail_;

static void*
conn_handler_thread(void* cl_info)
{
    struct client_info* info = (struct client_info*)cl_info;

    alog(LOG_LEVEL_INFO, "Handling connection from %s:%ld using fd %d",
         info->ipv4addr, info->port, info->sockfd);

    handle_http(info->sockfd);

    free(cl_info);
    sem_post(&tot_threads_avail_);
    return NULL;
}

void
set_max_connection_handler_threads(int n)
{
    sem_init(&tot_threads_avail_, 0, n);
}

void
handle_connection(struct client_info* cl_info)
{
    /*  1. clone the client_info */

    struct client_info* cl_info_copy = malloc(sizeof(struct client_info));

    *cl_info_copy = *cl_info;

    /*  2. create a new thread to handle connection */

    pthread_t http_handler;

    sem_wait(&tot_threads_avail_);
    if (pthread_create(&http_handler, NULL, &conn_handler_thread,
                       cl_info_copy) > 0)
        alog(LOG_LEVEL_ERROR, "Couldn't create a new http-handler thread");

    pthread_detach(http_handler); // avoid zombie threads
}
