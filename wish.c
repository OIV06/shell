#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#define MAX_ARGS 64
#define PATH_LEN 1024

// Simulating a simple "Shell" object with behaviors as functions.
void printError() {
    const char error_message[] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

char *default_paths[] = {"/bin", NULL}; // Default search path, simulating a property of our "Shell" object.
int pathNULL = 0; // Global flag to control execution of non-built-in commands.

char* findExecutable(char *command) {
    static char path[PATH_LEN];
    if (command[0] == '/' || command[0] == '.') {
        // If the command starts with '/' or '.', it's a path to an executable
        if (access(command, X_OK) == 0) {
            return command;
        } else {
            return NULL; // Executable at given path not found or not executable
        }
    } else if (pathNULL == 1) {
        return NULL; // If pathNULL is set, we shouldn't look for executables outside built-in commands
    } else {
        // Search for the command in the default_paths
        for (int i = 0; default_paths[i] != NULL; i++) {
            snprintf(path, PATH_LEN, "%s/%s", default_paths[i], command);
            if (access(path, X_OK) == 0) {
                return path;
            }
        }
        return NULL; // Executable not found in the paths
    }
}

void executeCommands(char *args[], int args_num) {
    if (strcmp(args[0], "exit") == 0) {
        if (args_num > 1) {
            printError(); // "exit" takes no arguments
        } else {
            exit(0);
        }
    } else if (strcmp(args[0], "cd") == 0) {
        if (args_num != 2) {
            printError();
        } else {
            if (chdir(args[1]) != 0) {
                printError();
            }
        }
    } else if (strcmp(args[0], "pathNULL") == 0) {
        if (args_num != 2 || (strcmp(args[1], "0") != 0 && strcmp(args[1], "1") != 0)) {
            printError();
        } else {
            pathNULL = atoi(args[1]);
        }
    } else {
        char *executablePath = findExecutable(args[0]);
        if (!executablePath) {
            printError();
            return;
        }

        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            if (execv(executablePath, args) == -1) {
                printError();
                exit(EXIT_FAILURE);
            }
        } else if (pid < 0) {
            // Failed to fork
            printError();
        } else {
            // Parent process
            waitpid(pid, NULL, 0);
        }
    }
}

int main(int argc, char *argv[]) {
    char *line = NULL;
    size_t bufsize = 0;

    // Setup the input stream based on if a file is given as argument
    FILE *input_stream = (argc == 1) ? stdin : fopen(argv[1], "r");
    if (input_stream == NULL) {
        fprintf(stderr, "wish: cannot open file\n");
        return 1;
    }

    printf("wish> "); // Print the prompt
    while (getline(&line, &bufsize, input_stream) != -1) {
        // Remove newline character from input
        line[strcspn(line, "\n")] = 0;

        // Tokenize the input line into arguments
        char *args[MAX_ARGS];
        int args_num = 0;
        char *part = strtok(line, " ");
        while (part != NULL) {
            args[args_num++] = part;
            part = strtok(NULL, " ");
        }
        args[args_num] = NULL; // NULL-terminate the argument list

        // Execute the parsed command
        executeCommands(args, args_num);

        // Reset line buffer
        free(line);
        line = NULL;
        bufsize = 0;

        // Print the prompt if reading from stdin
        if (argc == 1) {
            printf("wish> ");
        }
    }

    // Clean up
    free(line);
    if (argc > 1) {
        fclose(input_stream);
    }

    return 0;
}
