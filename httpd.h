//
// Created by tangtang on 17/2/9.
//

#include <evdns.h>

#ifndef MYHTTPD_HTTPD_H
#define MYHTTPD_HTTPD_H


int start_up(u_short *);
void err_exit(const char *);
void handle_request(void *);
int readline(int fd,char *buf,size_t size);
#endif //MYHTTPD_HTTPD_H
