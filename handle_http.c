#include "handle_http.h"
#include "async_log.h"
#include "exec_cgi.h"
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

/* Return numbers of characters read, or -1 on error
 */
static ssize_t
read_request_body(int sockfd, char* body, int* len)
{
}

static ssize_t
send_response_line(int sockfd, const char* line)
{
    ssize_t linelen = strlen(line);
    ssize_t nwritten = 0;
    ssize_t tot_written = 0;

    const char* pbuf = line;
    while (tot_written < linelen) {
        nwritten = write(sockfd, pbuf, linelen - tot_written);
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
url_to_local_file(const char* url_path, char* localfile, int len)
{
    ssize_t namelen = -1;

    /*  1. Local file prefix(wwwdir) */

    struct muQconf conf = get_conf();
    strcpy(localfile, conf.wwwdir);
    localfile += strlen(conf.wwwdir);

    /*  1. Find the location of first slash */

    char* first_slash = NULL;
    if ((first_slash = strchr(url_path, '/')) == NULL) {
        strcpy(localfile, "/index.html");
        return 11;
    }

    // Case that request url path contains only '/'
    if (strlen(first_slash) == 1) {
        strcpy(localfile, "/index.html");
        return 11;
    }

    /*  2. Copy the sequence of characters after first slash
     *     (including the first  slash)
     */

    namelen = strlen(first_slash);
    strncpy(localfile, first_slash, len);

    /*  3. If bad path */

    if (strstr(localfile, "../") != NULL)
        namelen = -1;

    return namelen;
}

static void
serve_file(int sockfd, int localfd)
{
    /* The last paramater of sendfile specify max bytes to send,
     * We want to send the whole file, so set it 102400bytes = 10MB
     */
    sendfile(sockfd, localfd, NULL, 100000);
}

void
handle_http(int sockfd)
{
    struct http_request_line reqline;
    memset(&reqline, 0, sizeof(struct http_request_line));
    char body[4096] = { 0 };

    /*  1. Is http? */

    if (read_request_line(sockfd, &reqline) < 2) {
        goto not_http;
    }

    alog(LOG_LEVEL_INFO, "Client HTTP request line: %s, %s, %s, %s",
         reqline.method, reqline.url_path, reqline.url_qstring,
         reqline.version);

    /*  2. Is GET or POST? */

    if (strcasecmp(reqline.method, "GET") != 0 &&
        strcasecmp(reqline.method, "POST") != 0) {
        goto not_implemented;
    }

    /*  3. Does responding local file found and can be opened? */

    int localfd;
    char localfile[512] = { 0 };

    if (url_to_local_file(reqline.url_path, localfile, 511) == -1) {
        goto bad_request;
    }

    if ((localfd = open(localfile, O_RDONLY)) == -1) {
        goto not_found;
    }

    /*  4. Static resources or CGI? */

    if (strcasecmp(localfile, "cgi-bin") == 0) {
        /* CGI */
        alog(LOG_LEVEL_INFO, "Client HTTP request cgi: %s", localfile);

        // FIXME: request body and CGI standard
        exec_cgi(localfd, reqline.url_qstring, sockfd);

    } else {
        /* Static resource */
        alog(LOG_LEVEL_INFO, "Know client HTTP request file: %s", localfile);

        send_response_line(sockfd, "HTTP/1.1 200 OK\r\n\r\n");
        serve_file(sockfd, localfd);

        alog(LOG_LEVEL_INFO, "Send file %s to client", localfile);
    }

    return;

not_http:
    alog(LOG_LEVEL_WARN,
         "Client doesn't send a well-formed HTTP request: %s, %s, %s, %s",
         reqline.method, reqline.url_path, reqline.url_qstring,
         reqline.version);
    send_response_line(sockfd, "HTTP/1.1 400 Bad Request\r\n\r\n");

    return;
bad_request:
    alog(LOG_LEVEL_WARN,
         "Client doesn't send a well-formed HTTP request: %s, %s, %s, %s",
         reqline.method, reqline.url_path, reqline.url_qstring,
         reqline.version);
    send_response_line(sockfd, "HTTP/1.1 400 Bad Request\r\n\r\n");

    return;
not_found:
    alog(LOG_LEVEL_INFO, "Client request file '%s' not on server", localfile);
    send_response_line(sockfd, "HTTP/1.1 404 Not Found\r\n\r\n");

    return;
not_implemented:
    alog(LOG_LEVEL_INFO, "Client request method '%s' not implemented",
         reqline.method);
    send_response_line(sockfd, "HTTP/1.1 501 Not Implemented\r\n\r\n");

    return;
}
