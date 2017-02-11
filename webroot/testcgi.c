//
// Created by tangtang on 17/2/10.
//

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char **argv){

    char buf[1024];
    char *queryString = getenv("REQUEST_METHOD");
    char *method = getenv("REQUEST_METHOD");
    char *contentLen = getenv("CONTENT_LENGTH");
    int content_len =0;

    snprintf(buf, sizeof(buf),"<html><head><title>test cgi</title></head><body>");
    write(STDOUT_FILENO,buf,strlen(buf));

    if (!method){
        snprintf(buf, sizeof(buf),"<h3>method: %s</h3></br>",method);
        write(STDOUT_FILENO,buf,strlen(buf));
    }

    if (!queryString){
        snprintf(buf, sizeof(buf),"<h3>querystr: %s</h3></br>",queryString);
        write(STDOUT_FILENO,buf,strlen(buf));
    }

    if (!contentLen){
        snprintf(buf, sizeof(buf),"<h3>contentLen: %s</h3></br>",contentLen);
        write(STDOUT_FILENO,buf,strlen(buf));

//        if ((content_len =atoi(contentLen))>0){
//            int readcount = sizeof(buf)-1>content_len?content_len: sizeof(buf)-1;
//            read(STDIN_FILENO,buf, readcount);
//            buf[readcount] ='\0';
//
//            snprintf(buf, sizeof(buf),"<h3>content: %s</h3></br>",buf);
//            write(STDOUT_FILENO,buf,strlen(buf));
//        }
    }



    snprintf(buf, sizeof(buf),"</body></html>");
    printf("%lu",strlen(buf));
    write(STDOUT_FILENO,buf,strlen(buf));

}