#ifndef EXEC_CGI_H
#define EXEC_CGI_H

/* Check whether the path is a CGI path
 * Return 1 if yes, 0 if not.
 */
int is_cgi_path(char* path);

/* Return -1 if CGI script not found, 0 otherwise */
int exec_cgi(char* localpath, char* method, char* qstring, int sockfd);

#endif
