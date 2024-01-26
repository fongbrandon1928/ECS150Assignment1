# Report

# sshell: Simple Shell
## 1. Summary
This program, `sshell` is a custom command line interpreter that can parse and 
execute basic commands such as cd, exit, pwd, and a custom sls command. It 
supports command pipes and output redirection. The code includes parsing user 
input, processing different types of commands, and the logic to execute these 
commands.

## 2. Check for >>
The redirection logic is altered to check for another '>' symbol after the 
first one to see if the file will be overwritten or appended to. Variable 
append is set to 1 if the second '>' symbol is present.


## 3. Hard Check for "'$'\n"
When doing any file redirection, whether the file name is blank or not, a file
would still be generated with "'$'\n" at the end of the name. We're not sure
why it is happening, but it is hard-coded to be appended from filename in the
code. The filename is also processed to be NULL if the user does not specify
a name to get a parse error.


## 4. sls method logic
This method checks for an existing directory and lists the files in said
directory and prints out the names and size of the files in parentheses. 

### while ((dir = readdir(d)) != NULL) { ... }
Inside the if block, this while loop iterates over the entries in the 
directory. readdir reads the next entry from the directory stream and returns 
a pointer to a struct dirent representing this entry. The loop continues until 
there are no more entries which means readdir returns NULL.

### stat(dir->d_name, &file_stat);
This line calls the stat function on the current file (or directory) entry. 
It fills the file_stat structure with information about the file, including its 
size.

### if (S_ISREG(file_stat.st_mode)) { ... }
This if statement checks whether the current entry is a regular file. S_ISREG 
is a macro that checks the file mode (stored in file_stat.st_mode) to 
determine if the entry is a regular file compared to something like a folder.

The directory is then closed at the end to free up space.


## 5. split_command method logic
This is the logic for how commands entered in the shell get processed.

The function takes four parameters:
- char *command: A string that contains the command to be split.

- char *commandStr[ARG_MAX]: An array of strings (character pointers) where 
the split parts of the command will be stored.

- char *split_indicator: 
A string that specifies the delimiter used for splitting the command.

- int *cmdNum: A pointer to an integer that will store the number of parts the 
command is split into.

The code uses strtok to split the cmd into multiple tokens where each token is
the string between each "|" which is the indicator for piping. These commands
are then stored into an array which will be referenced later in the code to
be performed later.


## 6. Implementation
The implementation of this program follows three distinct steps:
### 6.1 Parsing options
### 6.2 Pattern matching
#### 6.2.1 Pipe
An outside source was viewed and studied to learn how to control mutiple pipe 
redirection. The `pipe_cmds_count` stores the number of piped commands from 
cmdNum in `split_command(...,split_indicator_pipe, &cmdNum)`. From this, we 
need `pipe_cmds_count - 1` pipes, (for example command1 | command2, we just 
need `the number of commands - 1` pipes) for each pipe, we need two files to 
pipe them. `pipe[fd + i * 2]` piped every two files, `fd[2i]->pipe read end`, 
`fd[2i+1]->pipe write end` Then, step into creating processes using loop and 
fork and linking each pipe child to correct input and output.

```c
if  (i  !=  0){
dup2(fd[(i  -  1)  *  2],  STDIN_FILENO);
}
```
if it's not the first command, direct its STDIN to the Previous pipe write.
```c
if  (i  !=  pipe_cmds_count  -  1){
dup2(fd[i  *  2  +  1],  STDOUT_FILENO);
}
```
if it's not the last command, direct its STDOUT to current pipe read.  
![](image.png)

After doing all of these, the pipe children are already to use `execvp` to 
execute their commands. And the last child will print it output to terminal. 
If it fails it will still use `exit(EXIT_FAILURE)` to exit.

### 6.3 Output and error


## 7. Cite outside source
### 7.1 knowing how to control pipe redirection
`https://stackoverflow.com/questions/8389033/implementation-of-multiple-pipes
-in-c`
