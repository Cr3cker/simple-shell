#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_ARGS 64
#define MAX_CMD_LEN 256
#define MAX_HISTORY_SIZE 512


void handle_sigint(int sig) {
    printf("Gracefully exiting the shell...\n");
    exit(1);
}


void execute_pipes(char ***commands, size_t size) {
    int fd[2];
    pid_t pid;
    int in_fd = 0; 

    for (int i = 0; i < size; i++) {
        pipe(fd);

        pid = fork();
        if (pid == 0) {
            if (i != 0) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }

            if (i != size - 1) {
                dup2(fd[1], STDOUT_FILENO);
            }

            close(fd[0]); 

            if (execvp(commands[i][0], commands[i]) < 0) {
                perror("execvp");
                exit(1);
            }
        } else {
            wait(NULL);  
            if (i != 0) {
                close(in_fd);
            }
            in_fd = fd[0];
            if (i != size - 1) {
                close(fd[1]);
            }
        }
    }
}


char*** parse_command(char cmd[], size_t *size, int *pipe) {
    char *newline;
    char ***arr_of_commands;
    char *token;

    arr_of_commands = malloc(MAX_ARGS * sizeof(char **));

    newline = strchr(cmd, '\n');
    if (newline) {
        *newline = '\0';
    }

    int index = 0;

    if (strstr(cmd, "|") != NULL) {
        *pipe = 1;
        token = strsep(&cmd, "|");
        while (token != NULL) {
            char **arr_of_args = malloc(MAX_ARGS * sizeof(char *));
            int arg_index = 0;
            char *arg_token = strsep(&token, " ");
            while (arg_token != NULL) {
                if (arg_token[0] != '\0') { 
                    arr_of_args[arg_index] = arg_token;
                    arg_index++;
                }
                arg_token = strsep(&token, " ");
            }
            arr_of_args[arg_index] = NULL;
            arr_of_commands[index] = arr_of_args;
            token = strsep(&cmd, "|");
            index++;
        }
    } else {
        char **arr_of_args = malloc(MAX_ARGS * sizeof(char *));
        int arg_index = 0;
        token = strsep(&cmd, " ");
        while (token != NULL) {
            arr_of_args[arg_index] = token;
            token = strsep(&cmd, " ");
            arg_index++;
        }
        arr_of_args[arg_index] = NULL;
        arr_of_commands[index] = arr_of_args;
        index++;
    }

    arr_of_commands[index] = NULL;
    *size = index;

    return arr_of_commands;
}

void print_history(char *history[], int *history_index) {
    for (int i = 0; i < *history_index; i++) {
        printf("%s\n", history[i]);
    }
}

void add_to_history(char ***commands, size_t *size, char **history, int *history_index) {
    for (int i = 0; i < *size; i++) {
        if (*history_index < MAX_HISTORY_SIZE) {
            char *full_command = NULL;
            int j = 0;
            while (commands[i][j] != NULL) {
                size_t new_len = (full_command ? strlen(full_command) : 0) + strlen(commands[i][j]) + 2;
                full_command = realloc(full_command, new_len);
                strcat(full_command, commands[i][j]);
                strcat(full_command, " ");
                j++;
            }
            history[*history_index] = full_command;
            (*history_index)++;
        }
    }
}



int main() {
    signal(SIGINT, handle_sigint);
    char cmd[MAX_CMD_LEN];
    char *history[MAX_HISTORY_SIZE] = {0};
    int history_index = 0;
    size_t size;
    int pipe = 0;
    char ***commands;

    while (1) {
        printf("> ");
        fgets(cmd, MAX_CMD_LEN, stdin);
        commands = parse_command(cmd, &size, &pipe);

        if (size == 1 && strcmp(commands[0][0], "history") == 0) {
            print_history(history, &history_index);
            continue; 
        }

        execute_pipes(commands, size);

        add_to_history(commands, &size, history, &history_index);

        for (int i = 0; i < size; i++) {
            free(commands[i]);
        }
        free(commands);
    }
}