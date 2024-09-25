#include "main.h"

#include <stdio.h>

/* fork */
#include <sys/types.h>
#include <unistd.h>

/* wait */
#include <sys/types.h>
#include <sys/wait.h>

/* execve */
#include <unistd.h>

/* strtok */
#include <string.h>

/* realloc and malloc*/
#include <stdlib.h>

int main(int argc, char * argv[]) {

    #define CHILD_PROCESS_FAILURE 12
    #define EXECVE_FAILURE 11
    #define WAIT_FAILURE 10
    #define ENVIRONMENT_FAILURE 9
    #define PATH_PROBLEM 8

    /*prototypes*/
    char ** parse_command_line(char *);
    int handle_process_execution(char **);
    int change_directory(char **);
    void create_prompt(char *);

    char *buf = NULL;
    size_t count = 0;
    ssize_t nread;
    pid_t child_pid;
    int status;
    char * shell_name = "\n\033[1;34m[LAMBIONE'S SHELL]->[";

    while (1) {

        /* create the prompt every time*/
        create_prompt(shell_name);

        /* read user input */
        nread = getline(&buf, &count, stdin);
        if(nread == -1) {
            perror("Exiting shell");
            exit(1);
        } 
        
        /*parse command line*/
        char **command;
        command = (char **)parse_command_line(buf);

        /*get environment PATH for the executables*/
        char * path = getenv("PATH");
        if(path == NULL) {
            perror("No environment variable found");
            exit(ENVIRONMENT_FAILURE);
        }

        /*make a copy of the env paths*/
        char * path_dup = strdup(path);
        if(!path_dup) {
            exit(ENVIRONMENT_FAILURE);
        }

        /* make a copy of the first command user inputs*/
        char * first_cmd = strdup(command[0]);
        if(!first_cmd) {
            free(path_dup);
            exit(1);
        } 

        /*manage the built-in like the cd command*/
        if(strcmp(command[0], "cd") == 0) {
            /* change directory*/
            change_directory(command); 
            continue;
        }


        /* handle the executables like ls, ls -l etc..*/
        char * token;
        token = strtok(path_dup,":");
        while(token) {

            /* create child process */
            child_pid = fork();
            
            /* handle process execution */
            if (child_pid == -1) {
                free(path_dup);
                free(first_cmd);
                perror("Failed to create child process");
                exit(CHILD_PROCESS_FAILURE);
            }

            /* concatenate the environment path with the command */
            /* initialize command string space*/
            unsigned token_len = strlen(token);
            unsigned command_len = strlen(first_cmd);
            unsigned len = token_len + command_len;
            char * cmd;
            cmd = (char *)malloc(len + 2);
            if(!cmd) {
                free(path_dup);
                free(first_cmd);
                exit(1);
            }
            /*create the command string*/
            memcpy(cmd,token,token_len);
            cmd[token_len] = '/';
            memcpy(cmd+token_len +1,first_cmd,command_len);
            cmd[token_len + 1 + command_len] = '\0';

            /*replace in command first argument with the real executable*/
            command[0] = cmd;

            /* execute the process*/
            /* note -> if child process is successful on the execution of the process, it will continue so we need to check it's actual success iwith the parent*/
            int exec_trace = 0;
            if(child_pid == 0) {
                exec_trace = handle_process_execution(command);
            } else {
                /* parent waits for child otherwise*/   
                pid_t w = wait(&status);
                /* the parent checks if the child was successfull or not */
                if(exec_trace == 0) break;

                if(w == -1) {
                    free(path_dup);
                    free(first_cmd);
                    free(cmd);
                    exit(WAIT_FAILURE);
                }
            }  

            token = strtok(NULL,":");
            free(cmd);
        }
        free(path_dup);
        free(first_cmd);
        
    }
    free(buf);
    return 0;
}

void create_prompt(char * shell_name) {

    const char *white = "\033[1;37m";

    /*retrieve current directory*/
    char prompt_dir[1024];
    char * curr_dir = getcwd(prompt_dir, sizeof(prompt_dir));
    if (!curr_dir) {
        perror("something went wrong");
        exit(1);
    }

    /*reusable length*/
    int name_len = strlen(shell_name);
    int path_len = strlen(prompt_dir);
    int white_len = strlen(white);

    /*initialize memory for the prompt*/
    char * prompt = malloc(sizeof(char *) *name_len + sizeof(char *) *path_len + sizeof(char*) *white_len + 3);
    if (!prompt) {
        exit(1);
    }

    /*create the prompt*/
    memcpy(prompt, shell_name, name_len);
    memcpy(prompt + name_len,prompt_dir,path_len);
    prompt[name_len + path_len] = ']';
    prompt[name_len + path_len + 1] = '$';
    memcpy(prompt + name_len + path_len + 2,white,white_len);
    prompt[name_len + path_len + white_len + 3] = '\0';

    /* print shell prompt */
    write(STDOUT_FILENO,prompt,strlen(prompt));

}

/* handle the change of directory */
int change_directory(char ** command) {
    /* edge cases*/
    unsigned command_len = strlen(command);
    if(command_len <= 1) {
        perror("please specify the path");
        exit(PATH_PROBLEM);
    } 
    /* change the directory */
    char * path = command[1];
    if(chdir(path) != 0) {
        perror("no valid path");
        exit(PATH_PROBLEM);
    }
    return 0;
}

int handle_process_execution(char **command){
    /*execute process*/
    int execution_trace = 0;
    execution_trace = execve(command[0], command, NULL);
    return execution_trace;
}

char ** parse_command_line(char * buf){
    char **command;
    char *token;
    /* manage spaces and new lines*/
    token = strtok(buf," \n");
    /* initialize the command array */
    size_t size = 1024;
    command = (char **)malloc(sizeof(char *) * size);
    unsigned i = 0;
    while(token != NULL) {
        if (i >= size) {
            size *= 2;
            command = (char **)realloc(command, sizeof(char *) * size);
        }
        command[i] = token;
        token = strtok(NULL," \n");
        ++i;
    }
    // argv = {"ls", "-l", NULL} THE COMMAND ARRAY SHOULD BE NULL TERMINATED
    command[i] = NULL;
    return command;
}