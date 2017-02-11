//
// Created by tangtang on 17/2/11.
//

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc , char **argv){

    int pip[2];
    int pip2[2];
    pid_t pid;

    pipe(pip);
    pipe(pip2);

    if ((pid =fork())<0){
        exit(1);
    }

    if (pid ==0){
        close(pip[0]);
        dup2(pip[1],STDOUT_FILENO);
        close(pip2[1]);
        dup2(pip2[0],STDIN_FILENO);
        printf("hahah");
    }else {
        close(pip[1]);
        close(pip2[0]);

        char buf[1024];
        read(pip[0],buf, sizeof(buf));

        printf("%s",buf);
    }

    return 0;
}