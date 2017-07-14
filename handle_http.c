#include "handle_http.h"
#include "async_log.h"
#include "handle_cgi.h"
#include "read_conf.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

struct http_request_line
{
    char method[8];
    char url_path[1024];
    char url_qstring[1024];
    char version[9];
};

static ssize_t
read_request_line(int sockfd, struct http_request_line* line)
{
    char linebuf[4096] = { 0 };

    int tot_read = 0;

    /*  1. read request line */

    char c;
    int nread;
    while ((nread = read(sockfd, &c, 1)) > 0 && isprint(c)) {
        linebuf[tot_read++] = c;
    }
    linebuf[tot_read] = '\0';

    if (nread == -1) {
        alog(LOG_LEVEL_ERROR, "Read from socket %ld failed - %s:%ld",
             (long)sockfd, __FILE__, (long)__LINE__);
    }

    char path_and_qstring[2048] = { 0 };

    /*  2. scanf request line */

    int nitems;
    nitems = sscanf(linebuf, "%7s %2047s %8s", line->method, path_and_qstring,
                    line->version);

    char* qmark = strchr(path_and_qstring, '?');
    if (qmark == NULL) {
        strncpy(line->url_path, path_and_qstring, 1023);
    } else {
        int path_len = qmark - path_and_qstring;
        for (int i = 0; i < path_len && i < 2013; i++) {
            (line->url_path)[i] = path_and_qstring[i];
        }

        strncpy(line->url_qstring, qmark + 1, 1023);
        nitems += 1;
    }

    return nitems;
}

static ssize_t
send_response_str(int sockfd, const char* str)
{
    ssize_t len = strlen(str);
    ssize_t nwritten = 0;
    ssize_t tot_written = 0;

    const char* pbuf = str;
    while (tot_written < len) {
        nwritten = write(sockfd, pbuf, len - tot_written);
        if (nwritten == -1 && errno == EINTR)
            continue;
        else
            return -1;

        tot_written += nwritten;
        pbuf += nwritten;
    }

    /*  Must be 'linelen' bytes if we get here */

    return tot_written;
}

static ssize_t
url_to_local_path(const char* url_path, char* localpath, int len)
{
    ssize_t namelen = -1;

    /*  1. Local path prefix(wwwdir/) */

    struct muQconf conf = get_conf();
    strcpy(localpath, conf.wwwdir);
    localpath += strlen(conf.wwwdir);

    /*  1. Find the location of first slash */

    char* first_slash = NULL;
    if ((first_slash = strchr(url_path, '/')) == NULL) {
        strcpy(localpath, "/index.html");
        return 11;
    }

    // Case that request url path contains only '/'
    if (strlen(first_slash) == 1) {
        strcpy(localpath, "/index.html");
        return 11;
    }

    /*  2. Copy the sequence of characters after first slash
     *     (including the first  slash)
     */

    namelen = strlen(first_slash);
    strncpy(localpath, first_slash, len);

    /*  3. If bad path */

    if (strstr(localpath, "../") != NULL)
        namelen = -1;

    return namelen;
}

static int
serve_file(int sockfd, char* localpath)
{
    int fd;
    if ((fd = open(localpath, O_RDONLY)) == -1) {
        alog(LOG_LEVEL_ERROR, "Can't open file '%s'", localpath);
        return -1;
    }

    /* The last paramater of sendfile specify max bytes to send,
     * We want to send the whole file, so set it 100000bytes = 10MB
     */
    if (sendfile(sockfd, fd, NULL, 100000) != -1) {
        alog(LOG_LEVEL_INFO, "Send file '%s' to client", localpath);
        return 0;
    } else {
        alog(LOG_LEVEL_ERROR, "Can't send file '%s' to client", localpath);
        return -1;
    }

    // Never reach here
    return 0;
}

void
handle_http(int sockfd)
{
    struct http_request_line reqline;
    memset(&reqline, 0, sizeof(struct http_request_line));

    /*  1. Is http? */

    if (read_request_line(sockfd, &reqline) < 2) {
        goto bad_request;
    }

    alog(LOG_LEVEL_INFO, "Client HTTP request line: %s, %s, %s, %s",
         reqline.method, reqline.url_path, reqline.url_qstring,
         reqline.version);

    /*  2. Is GET or POST? */

    if (strcasecmp(reqline.method, "GET") != 0 &&
        strcasecmp(reqline.method, "POST") != 0) {
        goto not_implemented;
    }

    /*  3. Static resources or CGI scripts? */

    char localpath[512] = { 0 };

    if (url_to_local_path(reqline.url_path, localpath, 511) == -1) {
        goto bad_request;
    }

    if (is_cgi_path(localpath)) {
        alog(LOG_LEVEL_INFO, "Client HTTP request CGI script: %s", localpath);

        // Whatever, send OK
        send_response_str(sockfd, "HTTP/1.1 200 OK\r\n\r\n");
        if (exec_cgi(localpath, reqline.method, reqline.url_qstring, sockfd) ==
            -1) {
            alog(LOG_LEVEL_WARN, "CGI script '%s' not on server", localpath);

            char path_notfound[512];
            snprintf(path_notfound, 512, "%s/404_not_found.html",
                     get_conf().wwwdir);
            serve_file(sockfd, path_notfound);
        }

    } else {
        if (access(localpath, F_OK | R_OK) != 0) {
            goto not_found;
        }

        alog(LOG_LEVEL_INFO, "Client HTTP request file: %s", localpath);

        send_response_str(sockfd, "HTTP/1.1 200 OK\r\n\r\n");
        serve_file(sockfd, localpath);
    }

    return;

bad_request:
    alog(LOG_LEVEL_WARN,
         "Client doesn't send a well-formed HTTP request: %s, %s, %s, %s",
         reqline.method, reqline.url_path, reqline.url_qstring,
         reqline.version);
    send_response_str(sockfd, "HTTP/1.1 400 Bad Request\r\n\r\n");

    char path_badrequest[512];
    snprintf(path_badrequest, 512, "%s/400_bad_request.html",
             get_conf().wwwdir);
    serve_file(sockfd, path_badrequest);

    return;
not_found:
    alog(LOG_LEVEL_INFO, "Client request '%s' not on server", localpath);
    send_response_str(sockfd, "HTTP/1.1 404 Not Found\r\n\r\n");

    char path_notfound[512];
    snprintf(path_notfound, 512, "%s/404_not_found.html", get_conf().wwwdir);
    serve_file(sockfd, path_notfound);

    return;
not_implemented:
    alog(LOG_LEVEL_INFO, "Client request method '%s' not implemented",
         reqline.method);
    send_response_str(sockfd, "HTTP/1.1 501 Not Implemented\r\n\r\n");

    char path_notimplemented[512];
    snprintf(path_notimplemented, 512, "%s/501_not_implemented.html",
             get_conf().wwwdir);
    serve_file(sockfd, path_notimplemented);

    return;
}
