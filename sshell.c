#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define CMDLINE_MAX 512
#define Arg_MAX 64



int main(void){
        char cmd[CMDLINE_MAX];

        while (1) {
                char *nl;

                /* Print prompt */
                printf("sshell@ucd$ ");
                fflush(stdout);

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);

                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        printf("%s", cmd);
                        fflush(stdout);
                }
                int cmdNum = 0;
                char* cmdStr[Arg_MAX];
                int redirect = 0;
                int pipeindex = 0;
                char* filename = NULL;
                filename = strchr(cmd, '>');
                if (filename!= NULL) {
                        redirect = 1;
                        *filename = '\0';
                        filename++;
                }


                // split cmd with space
                char *token = strtok(cmd, " \n"); 
                // cmdStr gets these commands in a char* list
                // cmdNum gets the number of elements
                while (token != NULL) {
                        cmdStr[cmdNum++] = token; 
                        token = strtok(NULL, " \n"); 
                }
                cmdStr[cmdNum] = NULL;
                /* Remove trailing newline from command line */
                nl = strchr(cmdStr[0], '\n');
                if (nl)
                        *nl = '\0';
                if (!strcmp(cmdStr[0], "cd")) {
                        if (cmdStr[1] != NULL) {
                                if (chdir(cmdStr[1]) != 0) {
                                        perror("cd");
                                }
                        } 
                        else {
                                fprintf(stderr, "No path specified for cd\n");
                        }
                }
                

                /* Builtin command */
                else if (!strcmp(cmdStr[0], "exit")) {
                        fprintf(stderr, "Bye...\n");
                        break;
                }
                else if (!strcmp(cmdStr[0], "pwd")) {
                        char cwd[512];
                        if (getcwd(cwd, sizeof(cwd)) != NULL) {
                                printf("%s\n", cwd);  // print dic
                        } else {
                                perror("getcwd");  // error
                        }
                }
                else {
                        __pid_t pid;
                        pid = fork();
                        if (pid == 0) {
                                if (redirect && filename){
                                        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                                        if (fd < 0){
                                                perror("open");
                                                exit(EXIT_FAILURE);
                                                }
                                        dup2(fd, STDOUT_FILENO);
                                        close(fd);
                                }
                                execvp(cmdStr[0], cmdStr);
                                perror("execvp");
                                exit(EXIT_FAILURE);
                        }
                        else if (pid > 0) {
                                int status;
                                waitpid(pid, &status, 0);
                                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                                        printf("%s complete\n", cmdStr[0]);
                                }
                                else{
                                        fprintf(stderr, "child failed");
                                }
                        }
                        else {
                                perror("fork");
                                exit(1);
                        }
                }
        }
        return EXIT_SUCCESS;
}
