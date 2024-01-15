#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512

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
                char* cmdStr[64];

                // split cmd with space
                char *token = strtok(cmd, " \n"); 
                // cmdStr gets these commands in a char* list
                // cmdNum gets the number of elements
                while (token != NULL) {
                        cmdStr[cmdNum++] = token; 
                        token = strtok(NULL, " \n"); 
                }
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
                        break;
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
                        break;

                }
                else {
                        __pid_t pid;
                        pid = fork();
                        if (pid == 0) {
                                execvp(cmdStr[0], cmdStr);
                                perror("execvp");
                                exit(EXIT_FAILURE);
                        }
                        else if (pid > 0) {
                                int status;
                                waitpid(pid, &status, 0);
                                printf("%s complete\n", cmdStr[0]);
                                WEXITSTATUS(status);
                        }
                        else {
                                perror("fork");
                                exit(1);
                        }
                }
        }
        return EXIT_SUCCESS;
}
