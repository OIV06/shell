#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#define MAX_ARGS 64

void printError() {
    const char error_message[] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void executeCommands(char *args[], int args_num) {
    if (strcmp(args[0], "exit") == 0) {
        // Handling for "exit" command
        if (args_num > 1) {
            printError(); // "exit" takes no arguments
        } else {
            exit(0);
        }
    } else if (strcmp(args[0], "cd") == 0) {
        // Handling for "cd" command
        if (args_num != 2) {
            printError(); 
        } else {
            if (chdir(args[1]) != 0) {
                printError();
            }
        }
    } else {
        // Handling other commands with fork() and execv()
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            if (execv(args[0], args) == -1) {
                printError();
                exit(EXIT_FAILURE);
            }
        } else if (pid < 0) {
            // Forking failed
            printError();
        } else {
            // Parent process, wait for child to complete
            waitpid(pid, NULL, 0);
        }
    }
}

int main(int argc, char *argv[]) {
    char *line = NULL;
    size_t bufsize = 0;
    ssize_t lineSize;

    FILE *input_stream = (argc == 1) ? stdin : fopen(argv[1], "r");
    if (input_stream == NULL) {
        fprintf(stderr, "wish: cannot open file\n");
        return 1;
    }

    while ((lineSize = getline(&line, &bufsize, input_stream)) != -1) {
        if (line[lineSize - 1] == '\n') {
            line[lineSize - 1] = '\0';
        }

        char *args[MAX_ARGS];
        int args_num = 0;
        char *part = strsep(&line, " ");
        while (part != NULL && args_num < MAX_ARGS) {
            args[args_num++] = part;
            part = strsep(&line, " ");
        }
        args[args_num] = NULL; // NULL-terminate the argument list

        executeCommands(args, args_num);

        if (argc == 1) {
            printf("wish> ");
        }
    }

    if (lineSize == -1) {
        free(line);
        if (argc > 1) {
            fclose(input_stream);
        }
        exit(0); // Exit gracefully on EOF
    }

    return 0;
}

