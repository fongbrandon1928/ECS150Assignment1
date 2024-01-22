#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define CMDLINE_MAX 512

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
        char *cmdStr[64];
        int redirect = 0;
        char *filename = NULL;

        /* File redirect */
        char *token = strtok(cmd, " \n");
        while (token != NULL)
        {
            if (strcmp(token, ">") == 0)
            {
                redirect = 1;
                token = strtok(NULL, " \n");
                filename = token;
                break;
            }
            else
            {
                cmdStr[cmdNum++] = token;
            }
            token = strtok(NULL, " \n");
        }
        cmdStr[cmdNum] = NULL;
        
        /* Remove trailing newline from command line */
        nl = strchr(cmdStr[0], '\n');
        if (nl)
            *nl = '\0';

        for (int i = 0; i < cmdNum; i++)
        {
            if (strcmp(cmdStr[i], ">") == 0)
            {
                redirect = 1;
                cmdStr[i] = NULL;
                if (i + 1 < cmdNum)
                {
                    filename = cmdStr[i + 1];
                }
                break;
            }
        }
        cmdStr[cmdNum] = NULL;

	/* Complete message to print full command */
        char fullCmd[CMDLINE_MAX] = "";
        for (int i = 0; i < cmdNum; i++)
        {
            strcat(fullCmd, cmdStr[i]);
            if (i < cmdNum - 1)
            {
                strcat(fullCmd, " ");
            }
        }
        if (redirect && filename)
        {
            strcat(fullCmd, " > ");
            strcat(fullCmd, filename);
        }

        if (!strcmp(cmdStr[0], "cd"))
        {
            if (cmdStr[1] != NULL)
            {
                if (chdir(cmdStr[1]) != 0)
                {
                    perror("cd");
                }
            }
            else
            {
                fprintf(stderr, "No path specified for cd\n");
            }
        }

        /* Builtin command */
        else if (!strcmp(cmdStr[0], "exit"))
        {
            fprintf(stderr, "Bye...\n");
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

        else
        {
            __pid_t pid;
            pid = fork();
            if (pid == 0)
            {
                if (redirect && filename)
                {
                    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd < 0)
                    {
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
            else if (pid > 0)
            {
                int status;
                waitpid(pid, &status, 0);
                printf("+ complete '%s' [%d]\n", fullCmd, WEXITSTATUS(status));
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
