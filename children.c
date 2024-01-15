#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "children.h"

void childrenCalled(char* cmdStr[64]){
    __pid_t pid;
    pid = fork();
    if (pid == 0) {
        //execvp knows how to find the rest of the arguments
        execvp(cmdStr[0], cmdStr);
        perror("execvp");
        exit(EXIT_FAILURE);
        }
    else if (pid >0){
        int status;
        waitpid(pid, &status, 0);
        printf("%s complete\n", cmdStr[0]);
        WEXITSTATUS (status);
    }
    else{
        perror("fork");
        exit(1);
    }
}

