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
#include <errno.h>

/**
 * TODO 在线程中调用fork是不明知的选择，应该禁止
 * @param client
 * @param pRequest
 */

void execute_cgi(int client, request *pRequest);

void execute_file(int client, request *pRequest);

int main() {

    int ser_sock,client_sock;
    struct sockaddr_in client_addr;
    socklen_t socklen;
    u_short port;
    pthread_t thread;


    port = 0;
    ser_sock = start_up(&port);
    printf("listen port:%u",port);

    socklen = sizeof(client_addr);
    while (1){
        client_sock = accept(ser_sock,(struct sockaddr *)&client_addr,&socklen);

        if (client_sock<0){
            if (errno == EINTR) {
                continue;
            }
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
    request request;

    if(readline(client,buf, sizeof(buf))>0){
        if(parseStatusLine(&request,buf)!=0){
            snprintf(buf,sizeof(buf),"what this funck you");
            send(client,buf,strlen(buf),0);
            goto clean_up;
        }
    }

    printf("method: %s , protocol: %s , path: %s , querystring: %s",
           request.method,request.version,request.path,request.queryString?request.queryString:"");

    if (strlen(request.queryString)>0|| strcasecmp(request.method,"POST")==0){
        execute_cgi(client,&request);
    }else if (strcasecmp(request.method,"GET")!=0){
        not_implement(client);
    }else{
        execute_file(client,&request);
    }
clean_up:
    close(client);
}

void not_implement(int client){
    char buf[1024];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, ASERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}


void execute_file(int client, request *pRequest) {
    assert(pRequest);

    struct stat st;
    char buf[1024];

    char filepath[1024];
    getrequestFilePath(filepath, sizeof(filepath),pRequest->path);

    if (stat(filepath, &st) == -1) {
        while (readline(client, buf, sizeof(buf)) > 0 && strcmp(buf,"\n")!=0) {
        }

        not_found(client, pRequest->path);
        return;
    }

    if ((st.st_mode & S_IXUSR) ||
        (st.st_mode & S_IXGRP) ||
        (st.st_mode & S_IXOTH)){

        execute_cgi(client,pRequest);
        return;
    }

    headers(client, filepath);

    if (st.st_size>0)
        cat_file(client,filepath);
    //TODO 是否需要发送'\0
}

char * getrequestFilePath(char *filepath ,size_t size,/*out*/ const char *path){

    assert(filepath);
    assert(path);

    int pathLen = strlen(path);
    int filepathsize = size;
    if (pathLen == 0) {
        snprintf(filepath, sizeof(filepath), WEB_ROOT"/index.html");
    } else {
        snprintf(filepath,filepathsize,WEB_ROOT);
        strncat(filepath,path,filepathsize-strlen(filepath));

        if (filepath[strlen(filepath)-1] == '/') {
            strncat(filepath,"index.html",filepathsize-strlen(filepath));
        }
    }

    return filepath;
}

void headers(int fd, const char *filename){
    char buf[1024];
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(fd,buf,strlen(buf),0);
    strcpy(buf, ASERVER_STRING);
    send(fd,buf,strlen(buf),0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(fd,buf,strlen(buf),0);
    strcpy(buf, "\r\n");
    send(fd,buf,strlen(buf),0);

}


void cat_file(int client, const char *path){

    char buf[1024];
    FILE *file = fopen(path,"r");

    if (!file)
        return;

    fgets(buf, sizeof(buf),file);
    while (!feof(file)){
        send(client,buf, strlen(buf),0);
        fgets(buf, sizeof(buf), file);
    }

    send(client,"\0", sizeof(char),0);
    fclose(file);
}

void not_found(int fd,const char *path){

    assert(path);

    char buf[1024];
    strcpy(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(fd,buf,strlen(buf),0);
    strcpy(buf, ASERVER_STRING);
    send(fd,buf,strlen(buf),0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(fd,buf,strlen(buf),0);
    strcpy(buf, "\r\n");
    send(fd,buf,strlen(buf),0);

    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><H1>Not FOUND</H1></P>\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "your request a fake website\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "你请求了个假网站.\r\n");
    send(fd, buf, strlen(buf), 0);
    sprintf(buf,"%s",path);
    send(fd, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(fd, buf, strlen(buf), 0);
}

void execute_cgi(int client, request *pRequest) {

    char buf[1024];
    int n =1,status;
    int contentLen =0;
    int out_pipe[2]; //读入数据
    int in_pipe[2]; //写出数据
    pid_t  pid;
    char c;

    //get 需要QUERY_STRING＝
    //post 需要CONTENT_LENGTH
    if (strcasecmp(pRequest->method,"POST")==0) {
        do{
            n= readline(client,buf, sizeof(buf));
            if (strcasecmp(buf,"Content-Length:") ==0 && strlen(buf)>=17){
                contentLen = atoi(&buf[16]);
            }
        } while (n>0&& strcmp(buf,"\n")!=0);

        if (contentLen<=0){
            if (contentLen == -1) {
                bad_request(client);
                return;
            }
        }

    } else { //GET
        do{
            n= readline(client,buf, sizeof(buf));
        } while (n>0&& strcmp(buf,"\n")!=0);
    }



    if (pipe(in_pipe)==-1){
        cannot_execute(client);
        return;
    }

    if (pipe(out_pipe)==-1){
        cannot_execute(client);
        return;
    }

    if ((pid = fork())<0){
        cannot_execute(client);
        return;
    }


    if (pid == 0){ //子线程

        char filePath[1024];

        char oo[1024];

        close(out_pipe[0]);
        dup2(out_pipe[1],STDOUT_FILENO);
        close(in_pipe[1]);
        dup2(in_pipe[0],STDIN_FILENO);
        printf("hahah");

        close(out_pipe[0]);
        close(in_pipe[1]);
        dup2(out_pipe[1],STDOUT_FILENO);
        dup2(in_pipe[0],STDIN_FILENO);


//        sprintf(oo,"%d   %d",STDOUT_FILENO,STDIN_FILENO);
//        FILE *file = fopen("/Users/tangtang/ClionProjects/myhttpd/webroot/testcgiss","w");
//        fputs(oo,file);
//        fclose(file);






        getrequestFilePath(filePath, sizeof(filePath),pRequest->path);
        snprintf(buf, sizeof(buf),"REQUEST_METHOD=%s",pRequest->method);
        putenv(buf);

        if (strcasecmp(pRequest->method,"GET")==0){
            snprintf(buf, sizeof(buf),"QUERY_STRING＝%s", pRequest->queryString);
            putenv(buf);
        }else {
            snprintf(buf, sizeof(buf),"CONTENT_LENGTH＝%d", contentLen);
            putenv(buf);
        }
                FILE *file = fopen("/Users/tangtang/ClionProjects/myhttpd/webroot/testcgiss","w");
        fputs(oo,file);
        fclose(file);

        printf("hhaahhh");
//        execl(filePath, NULL);
        exit(0);
    } else{
        close(out_pipe[1]);
        close(in_pipe[0]);
        sprintf(buf, "HTTP/1.0 200 OK\r\n");
        send(client, buf, strlen(buf), 0);
        //何时提前断开连接？
        if (strcasecmp(pRequest->method,"POST") ==0){
            for (int i = 0; i < contentLen; ++i) {
                n = recv(client,&c, sizeof(c),0);
                if (n>0){
                    send(in_pipe[1],&c, sizeof(c),0);
                }
            }
        }

        while (read(out_pipe[0], &c, sizeof(c)) > 0)
            send(client, &c, 1, 0);

        perror("read");

        close(out_pipe[0]);
        close(in_pipe[1]);
        waitpid(pid, &status, 0);
    }
}


void cannot_execute(int client) {
    char buf[1024];

    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error perl CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}

void bad_request(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

int parseStatusLine(request *request,const char *buf) {
    assert(request);
    assert(buf);

    int i =0,pos =0;

    while (!isspace(buf[i])&&buf[i]!='\0'){
        request->method[pos++] = buf[i++];
    }

    request->method[pos] = '\0';
    if (buf[i] == '\0'){
        return  -1;
    }

    pos =0;
    i++;
    while (!isspace(buf[i])&&buf[i]!='\0'&&buf[i]!='?'){
        request->path[pos++] = buf[i++];
    }
    request->path[pos] = '\0';
    pos =0;

    if (buf[i]=='?'){
        i++;
        while (!isspace(buf[i])&&buf[i]!='\0'){
            request->queryString[pos++] = buf[i++];
        }
    }

    request->queryString[pos] ='\0';

    if (buf[i] == '\0'){
        return  -1;
    }

    i++;
    pos =0;
    while (!isspace(buf[i])&&buf[i]!='\0'&&buf[i]!='\n'){
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


    do {
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
    }while (count<size-1 && c!='\n');

    buf[count++]='\0';

    return count;
}
