#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define CMDLINE_MAX 512
#define ARG_MAX 64

void split_command(char *command, char *commandStr[ARG_MAX], char *split_indicator, int *cmdNum)
{
    *cmdNum = 0;

    // split cmd with space
    char *token = strtok(command, split_indicator);
    // cmdStr gets these commands in a char* list
    // cmdNum gets the number of elements
    while (token != NULL)
    {
        commandStr[*cmdNum] = token;
        (*cmdNum)++;
        token = strtok(NULL, split_indicator);
    }
    commandStr[*cmdNum] = NULL;
}

int main(void)
{
    char cmd[CMDLINE_MAX];

    while (1)
    {
        char *nl;

        /* Print prompt */
        printf("sshell@ucd$ ");
        fflush(stdout);

        /* Get command line */
        fgets(cmd, CMDLINE_MAX, stdin);

        /* Print command line if stdin is not provided by terminal */
        if (!isatty(STDIN_FILENO))
        {
            printf("%s", cmd);
            fflush(stdout);
        }

        int cmdNum = 0;
        char *cmdStr[ARG_MAX] = {NULL};
        int redirect = 0;
        int pipe_start = 0;
        char split_indicator[] = " \n";
        char split_indicator_pipe[] = "|";
        char *filename = NULL;
        char *pipe_cmds[ARG_MAX] = {NULL};
        char *pipe_cmds_str[ARG_MAX][ARG_MAX] = {0};
        int pipe_cmds_count = 0;
        filename = strchr(cmd, '>');
        pipe_cmds[0] = strchr(cmd, '|');

        /* check if it redirects to any other file first*/
        if (filename != NULL)
        {
            redirect = 1;
            /* split command in two part: cmd without redirection and filename*/
            *filename = '\0';
            filename++;
        }

        if (pipe_cmds[0] != NULL)
        {
            pipe_start = 1;
            /* split with '|' first*/
            split_command(cmd, cmdStr, split_indicator_pipe, &cmdNum);
            /* record how many commands piping*/
            pipe_cmds_count = cmdNum;
            if (cmdNum <= 1)
            {
                if (redirect)
                {
                    fprintf(stderr, "Error: mislocated output direction\n");
                    continue;
                }
                else
                {
                    fprintf(stderr, "Error: missing command\n");
                    continue;
                }
            }
            /* split each string list in pipe_cmds with space and put it in pipe_cmds_str*/
            for (int i = 0; i < pipe_cmds_count; i++)
            {
                pipe_cmds[i] = cmdStr[i];
                split_command(pipe_cmds[i], pipe_cmds_str[i], split_indicator, &cmdNum);
                if (strlen(pipe_cmds[i]) == 1)
                {
                    fprintf(stderr, "Error: missing command\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
        else
        {
            /* split with space*/
            split_command(cmd, cmdStr, split_indicator, &cmdNum);
            if (cmdNum > 2)
            {
                /* check if ls/cd command has more than 1 argument */
                if (strcmp(cmd, "ls") == 0 || strcmp(cmd, "cd") == 0)
                {
                    fprintf(stderr, "Error: too many process arguments\n");
                    continue;
                }
            }
            else if (cmdNum == 0)
            {
                fprintf(stderr, "Error: missing command\n");
                continue;
            }
        }

        /* Remove trailing newline from command line */
        nl = strchr(cmdStr[0], '\n');
        if (nl)
            *nl = '\0';

        /* Builtin command */
        if (!strcmp(cmdStr[0], "cd"))
        {
            if (cmdStr[1] != NULL)
            {
                if (chdir(cmdStr[1]) != 0)
                {
                    fprintf(stderr, "Error: cannot cd into directory\n");
                }
            }
            else
            {
                fprintf(stderr, "No path specified for cd\n");
            }
        }

        else if (!strcmp(cmdStr[0], "exit"))
        {
            fprintf(stderr, "Bye...\n");
            memset(cmd, 0, CMDLINE_MAX);
            break;
        }
        else if (!strcmp(cmdStr[0], "pwd"))
        {
            char cwd[CMDLINE_MAX];
            if (getcwd(cwd, sizeof(cwd)) != NULL)
            {
                printf("%s\n", cwd); // print dic
            }
            else
            {
                perror("getcwd"); // error
            }
        }
        /* system command*/
        else
        {
            __pid_t pid;
            pid = fork();
            if (pid == 0)
            {
                /* redirect output*/
                if (redirect && filename)
                {
                    if (strlen(filename) == 1)
                    {
                        fprintf(stderr, "Error: no output file\n");
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (fd < 0)
                        {
                            fprintf(stderr, "Error: cannot open output file\n");
                            exit(EXIT_FAILURE);
                        }
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                    }
                }
                /*
                outside website studied:
                knowing how to control pipe redirection
                https://stackoverflow.com/questions/8389033/implementation-of-multiple-pipes-in-c
                see report
                */
                if (pipe_start)
                {
                    /* each pipe needs two files*/
                    int fd[2 * (pipe_cmds_count - 1)];

                    /* creating pipes*/
                    for (int i = 0; i < pipe_cmds_count - 1; i++)
                    {
                        /*pipe every two files*/
                        if (pipe(fd + i * 2) < 0)
                        {
                            perror("pipe");
                            exit(EXIT_FAILURE);
                        }
                    }

                    /* creating processes*/
                    for (int i = 0; i < pipe_cmds_count; i++)
                    {
                        if (fork() == 0)
                        {
                            /* if it's not the first command, direct its STDIN to the Previous pipe write*/
                            if (i != 0)
                            {
                                dup2(fd[(i - 1) * 2], STDIN_FILENO);
                            }

                            /* if it's not the last command, direct its STDOUT to current pipe read*/
                            if (i != pipe_cmds_count - 1)
                            {
                                dup2(fd[i * 2 + 1], STDOUT_FILENO);
                            }

                            /* close all the files*/
                            for (int j = 0; j < 2 * (pipe_cmds_count - 1); j++)
                            {
                                close(fd[j]);
                            }

                            /* execute commands*/
                            execvp(pipe_cmds_str[i][0], pipe_cmds_str[i]);
                            perror("execvp");
                            exit(EXIT_FAILURE);
                        }
                    }

                    /* close all files in parent process*/
                    for (int i = 0; i < 2 * (pipe_cmds_count - 1); i++)
                    {
                        close(fd[i]);
                    }

                    /* wait all children*/
                    for (int i = 0; i < pipe_cmds_count; i++)
                    {
                        wait(NULL);
                    }
                    exit(0);
                }
                else
                {
                    execvp(cmdStr[0], cmdStr);
                    fprintf(stderr, "Error: command not found\n");
                    exit(EXIT_FAILURE);
                }
            }
            else if (pid > 0)
            {
                int status;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                {
                    printf("+ complete '%s'\n", cmdStr[0]);
                }
                else
                {
                    fprintf(stderr, "child failed\n");
                }
            }
            else
            {
                perror("fork");
                exit(1);
            }
        }
    }
    return EXIT_SUCCESS;
}
