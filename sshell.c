#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <dirent.h>

#define CMDLINE_MAX 512
#define ARG_MAX 64

void split_command(char *command, char *commandStr[ARG_MAX], char *split_indicator, int *cmdNum)
{
    *cmdNum = 0;

    /* Split command with space */
    char *token = strtok(command, split_indicator);
    /* cmdStr gets these commands in a char* list */
    /* cmdNum gets the number of elements */
    while (token != NULL)
    {
        commandStr[*cmdNum] = token;
        (*cmdNum)++;
        token = strtok(NULL, split_indicator);
    }
    commandStr[*cmdNum] = NULL;
}

/* Get files and their size in directory */
void sls(void) {
	DIR *d;
    struct dirent *dir;
    struct stat file_stat;
    d = opendir(".");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            stat(dir->d_name, &file_stat);
            /* Check if it is a regular file */
            if (S_ISREG(file_stat.st_mode)) {
                printf("%s (%ld bytes)\n", dir->d_name, file_stat.st_size);
            }
        }
        closedir(d);
    } else {
        fprintf(stderr, "Error: cannot open directory");
    }
}

int main(void)
{
    char cmd[CMDLINE_MAX];
    char statusList[4];

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

        char fullCmd[CMDLINE_MAX];
        int cmdNum = 0;
        char *cmdStr[ARG_MAX] = {NULL};
        int redirect = 0;
        int pipe_start = 0;
        char split_indicator[] = " \n";
        char split_indicator_pipe[] = "|";
        char *filename = NULL;
        char *filename_with_newline = strchr(cmd, '>');
        char *pipe_cmds[ARG_MAX] = {NULL};
        char *pipe_cmds_str[ARG_MAX][ARG_MAX] = {0};
        int pipe_cmds_count = 0;
        int append = 0;

        strcpy(fullCmd, cmd);
        fullCmd[strlen(fullCmd) - 1] = '\0';

        /* Parse if empty filename */
        if (filename_with_newline)
        {
        	if (*(filename_with_newline + 1 ) == '>')
        	{
        		append = 1;
        		*filename_with_newline = '\0';
        		filename = filename_with_newline + 2;
        		filename = strtok(filename, "'$'\n");
        	}
        	else 
        	{
            	*filename_with_newline = '\0';
            	filename = filename_with_newline + 1;
            	filename = strtok(filename, "'$'\n");
            }
            if (filename && isspace((unsigned char)*filename) && strlen(filename) <= 1)
            {
                filename = NULL;
            }
        }
        pipe_cmds[0] = strchr(cmd, '|');

        /* Check for redirection to any other file first*/
        if (filename != NULL)
        {
            if (strchr(filename, '|'))
            {
                fprintf(stderr, "Error: mislocated output direction\n");
                continue;
            }
            else
            {
                redirect = 1;
                /* Split command in two parts: cmd without redirection and filename */
                *filename = '\0';
                filename++;
            }
        }

        if (pipe_cmds[0] != NULL)
        {
            pipe_start = 1;
            /* split with '|' first */
            split_command(cmd, cmdStr, split_indicator_pipe, &cmdNum);
            /* Record how many commands piping */
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
            /* Split each string list in pipe_cmds with space and put it in pipe_cmds_str */
            for (int i = 0; i < pipe_cmds_count; i++)
            {
                pipe_cmds[i] = cmdStr[i];
                split_command(pipe_cmds[i], pipe_cmds_str[i], split_indicator, &cmdNum);
                if (strlen(pipe_cmds[i]) == 1)
                {
                    fprintf(stderr, "Error: missing command\n");
                    continue;
                }
            }
        }
        else
        {
            /* Split cmd with space */
            split_command(cmd, cmdStr, split_indicator, &cmdNum);
            if (cmdNum > ARG_MAX)
            {
                /* Check if ls/cd command has more than ARG_MAX arguments */
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
        /* cd */
        if (!strcmp(cmdStr[0], "cd"))
        {
            if (cmdStr[1] != NULL)
            {
                if (chdir(cmdStr[1]) != 0)
                {
                    fprintf(stdout, "Error: cannot cd into directory\n");
                }
            }
            else
            {
                fprintf(stderr, "No path specified for cd\n");
            }
            fprintf(stderr, "+ completed '%s' [0]\n", fullCmd);
        }
		
        /* sls */
		else if (!strcmp(cmdStr[0], "sls")) {
			sls();
			fprintf(stderr, "+ completed 'sls' [0]\n");
		}
		
        /* exit */
        else if (!strcmp(cmdStr[0], "exit"))
        {
            fprintf(stderr, "Bye...\n");
            fprintf(stderr,"+ completed 'exit' [0]\n");
            break;
        }
        /* pwd */
        else if (!strcmp(cmdStr[0], "pwd"))
        {
            char cwd[CMDLINE_MAX];
            if (getcwd(cwd, sizeof(cwd)) != NULL)
            {
                fprintf(stdout, "%s\n", cwd);
                fprintf(stderr, "+ completed 'pwd' [0]\n");
            }
            else
            {
                perror("getcwd");
            }
        }
        /* System command */
        else
        {
            __pid_t pid;
            pid = fork();
            if (pid == 0)
            {
                /* Redirect output */
                if (redirect && filename)
                {
                	int fd;
					if (append)
                    	fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    else
                    	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd < 0)
                    {
                        fprintf(stderr, "Error: cannot open output file\n");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }
                
                /* outside website studied:
                knowing how to control pipe redirection
                https://stackoverflow.com/questions/8389033/implementation-of-multiple-pipes-in-c
                see report */
                
                if (pipe_start)
                {
                    /* Each pipe needs two files */
                    int fd[2 * (pipe_cmds_count - 1)];
                    pid_t pid[pipe_cmds_count];

                    /* Creating pipes*/
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
                        pid[i] = fork();
                        if (pid[i] == 0)
                        {
                            /* If not on first command, direct STDIN to the previous pipe write*/
                            if (i != 0)
                            {
                                dup2(fd[(i - 1) * 2], STDIN_FILENO);
                            }

                            /* If not on last command, direct STDOUT to current pipe read*/
                            if (i != pipe_cmds_count - 1)
                            {
                                dup2(fd[i * 2 + 1], STDOUT_FILENO);
                            }

                            /* Close all the files */
                            for (int j = 0; j < 2 * (pipe_cmds_count - 1); j++)
                            {
                                close(fd[j]);
                            }

                            /* Execute commands */
                            execvp(pipe_cmds_str[i][0], pipe_cmds_str[i]);
                            perror("execvp");
                            exit(EXIT_FAILURE);
                        }
                        else if (pid[i] < 0) {
                            perror("fork");
                            exit(EXIT_FAILURE);
                        }
                    }

                    /* Close all files in parent process */
                    for (int i = 0; i < 2 * (pipe_cmds_count - 1); i++)
                    {
                        close(fd[i]);
                    }

                    /* Wait for all children */
                    for (int i = 0; i < pipe_cmds_count; i++)
                    {

                        int status;
                        waitpid(pid[i], &status, 0);
                        statusList[i] = WEXITSTATUS(status);

                    }

                    /* Print complete message if pipe */
                    fprintf(stderr, "+ completed '%s' ", fullCmd);
                    for (int i = 0; i < pipe_cmds_count; i++)
                    {
                        printf("[%d]", statusList[i]);
                    }
                    printf("\n");

                    exit(EXIT_FAILURE);
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

                /* Print for single command */
                if (pipe_cmds_count == 0)
                {
                    fprintf(stderr, "+ completed '%s' [%d]\n", fullCmd, WEXITSTATUS(status));
                }
            }
            else
            {
                perror("fork");
                exit(EXIT_FAILURE);
            }
        }
    }
    return EXIT_SUCCESS;
}
