//
// Created by tangtang on 17/2/9.
//

#include <evdns.h>

#ifndef MYHTTPD_HTTPD_H
#define MYHTTPD_HTTPD_H

#define WEB_ROOT "webroot"

typedef struct request{
    char method[255];
    char version[10];
    char path[255];
    char queryString[1024];

}request;

int start_up(u_short *);
void err_exit(const char *);
void handle_request(void *);
int readline(int fd,char *buf,size_t size);
int parseStatusLine(request *, const char *);
void execute_file(int client, request *pRequest);
void execute_cgi(int client, request *pRequest);
void cat_file(int , const char * );
void not_found(int ,const char *);

#endif //MYHTTPD_HTTPD_H
