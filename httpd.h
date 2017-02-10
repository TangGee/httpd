//
// Created by tangtang on 17/2/9.
//

#include <evdns.h>

#ifndef MYHTTPD_HTTPD_H
#define MYHTTPD_HTTPD_H

#define WEB_ROOT "webroot"
#define ASERVER_STRING "xhttpd"

typedef struct request{
    char method[255];
    char version[10];
    char path[255];
    char queryString[1024];

}request;

int start_up(u_short *);
void err_exit(const char *);
void handle_request(void *);
int readline(int ,char *,size_t );
int parseStatusLine(request *, const char *);
void execute_file(int , request *);
void execute_cgi(int , request *);
void cat_file(int , const char * );
void not_found(int ,const char *);
void headers(int , const char *);

#endif //MYHTTPD_HTTPD_H
