#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include "httpd.h"
#include <sys/stat.h>

void execute_cgi(int client, request *pRequest);

void execute_file(int client, request *pRequest);

int main() {

    int ser_sock,client_sock;
    struct sockaddr_in client_addr;
    socklen_t socklen;
    u_short port;
    pthread_t thread;


    port = 3000;
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
    }

    close(ser_sock);
    return 0;
}

void handle_request(void *arg){
    int client = *(int * )arg;
    char buf[1024];
    char method[255];
    request request;

    if(readline(client,buf, sizeof(buf))>0){
        if(parseStatusLine(&request,buf)!=0){
            snprintf(buf,sizeof(buf),"what this funck you");
            send(client,buf,strlen(buf)+1,0);
            goto clean_up;
        }
    }

    printf("method: %s , protocol: %s , path: %s , querystring: %s",
           request.method,request.version,request.path,request.queryString?request.queryString:"");

    if (strlen(request.queryString)>0|| strcasecmp(request.method,"POST")){
        execute_cgi(client,&request);
    }else {
        execute_file(client,&request);
    }
clean_up:
    close(client);
}

void execute_file(int client, request *pRequest) {
    assert(pRequest);

    struct stat st;
    char buf[1024];

    char filepath[1024];
    int pathLen = strlen(pRequest->path);
    if (pathLen == 0) {
        snprintf(filepath, sizeof(filepath), WEB_ROOT
                "/index.html");
    } else {
        int count = snprintf(filepath, sizeof(filepath), WEB_ROOT
                "/", pRequest->path);
        int limit = sizeof(filepath) - count;
        if (filepath[count - 1] == '/') {
            snprintf(filepath, limit, "index.html");
        }
    }

    if (stat(&filepath, &st) == -1) {
        while (readline(client, buf, sizeof(buf)) > 0) {
        }

        not_found(client, &pRequest->path);
        return;
    }

    if ((st.st_mode & S_IXUSR) ||
        (st.st_mode & S_IXGRP) ||
        (st.st_mode & S_IXOTH)){

        execute_cgi(client,pRequest);
        return;
    }

    //TODO header

    if (st.st_size>0)
        cat_file(client,&filepath);
}


void cat_file(int client, const char *path){

    char buf[1024];
    FILE *file = fopen(path,'r');

    if (!file)
        return;

    fgets(buf, sizeof(buf),file);
    while (feof(file)){
        send(client,buf, strlen(buf),0);
        fgets(buf, sizeof(buf), file);
    }

    send(client,'\0', sizeof(char),0);
    close(file);
}

void not_found(int ,const char *);

void execute_cgi(int client, request *pRequest) {

}

int parseStatusLine(request *request,const char *buf) {
    assert(request!=NULL);
    assert(buf!=NULL);

    int i =0,pos =0;

    while (!isspace(buf[i])&&buf[i]!=NULL){
        request->method[pos++] = buf[i++];
    }

    request->method[pos] = '\0';
    if (buf[i] == NULL){
        return  -1;
    }

    pos =0;
    i++;
    while (!isspace(buf[i])&&buf[i]!=NULL&&buf[i]!='?'){
        request->path[pos++] = buf[i++];
    }
    request->path[pos] = '\0';
    pos =0;

    if (buf[i]=='?'){
        i++;
        while (!isspace(buf[i])&&buf[i]!=NULL){
            request->queryString[pos++] = buf[i++];
        }
    }

    request->queryString[pos] ='\0';

    if (buf[i] == NULL){
        return  -1;
    }

    i++;
    pos =0;
    while (!isspace(buf[i])&&buf[i]!=NULL&&buf[i]!='\n'){
        request->version[pos++] = buf[i++];
    }
    request->version[pos] = '\0';

    return 0;
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