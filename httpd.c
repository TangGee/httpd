#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include "httpd.h"

int main() {

    int ser_sock,client_sock;
    struct sockaddr_in client_addr;
    socklen_t socklen;
    u_short port;
    pthread_t thread;


    port = 5000;
    ser_sock = start_up(&port);
    printf("listen port:%u",port);

    socklen = sizeof(client_addr);
    while (1){
        client_sock = accept(ser_sock,(struct sockaddr *)&client_addr,&socklen);
        if (client_sock<0){
            err_exit("accept");
        }

        if (pthread_create(&thread,NULL,(void *)handle_request,(void *)&client_sock)!=0){
            err_exit("thread_create");
        }

        close(client_sock);
    }

    close(ser_sock);
    return 0;
}

void handle_request(void *arg){
    int client = *(int * )arg;
    char buf[1024];

    while (readline(client,buf, sizeof(buf))>0){
        printf("%s",buf);
    }

    close(client);
}

int start_up(u_short *port){

    int sock;
    struct sockaddr_in addr;
    socklen_t  socklen;
    int on =1;

    sock = socket(PF_INET,SOCK_STREAM,0);

    if (sock<0){
        err_exit("socket");
    }

    memset(&addr,0, sizeof(addr));
    addr.sin_port = htons(*port);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    socklen = sizeof(addr);
    if (bind(sock,(struct sockaddr *)&addr,socklen)==-1){
        err_exit("bind");
    }

    if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&on, sizeof(on)) ==-1){
        err_exit("setsockopt");
    }

    if (*port ==0){
        if (getsockname(sock,(struct sockaddr *)&addr,&socklen) ==-1){
            err_exit("getsockname");
        }

        *port = ntohs(addr.sin_port);
    }

    if (listen(sock,5) ==-1){
        err_exit("getsockname");
    }

    return sock;


}
void err_exit(const char *err){
    perror(err);
    exit(1);
}

int readline(int fd,char *buf,size_t size){

    char c=0;
    int n;

    int count =0;

    n = recv(fd,&c,sizeof(c),0);
    if (n<0){
        return -1;
    }
    buf[count++]= c;

    while (count<size-1 && c!='\n'){
        n = recv(fd,&c, sizeof(c),0);
        if (n>0){
            if (c=='\r'){
                n = recv(fd,&c,sizeof(c),MSG_PEEK);
                if (n>0&& c =='\n'){
                    recv(fd,&c,sizeof(c),0);
                }else{
                    c ='\n';
                }
            }

        }else{
            c = '\n';
        }
        buf[count++] = c;
    }
    buf[count++]='\0';

    return count;
}