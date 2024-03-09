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
int isTestSequence(const char* command) {
    return strncmp(command, "test", 4) == 0;
}
char *default_paths[] = {"/bin", NULL}; // Default search path, simulating a property of our "Shell" object.

// Method to search for an executable in the default paths.
char* findExecutable(char *command) {//
    static char path[PATH_LEN];
    if (command[0] == '/' || command[0] == '.') {
        // Command already has a path or is relative to the current directory
        return command;
    } else {
        // Search in the default paths
        for (int i = 0; default_paths[i] != NULL; i++) {
            snprintf(path, PATH_LEN, "%s/%s", default_paths[i], command);
            if (access(path, X_OK) == 0) {
                return path;
            }
        }
    }
    return NULL; // Executable not found
}

void executeCommands(char *args[], int args_num, char ***paths) {
    if (strcmp(args[0], "exit") == 0) {
        if (args_num > 1) {
            printError(); // "exit" takes no arguments
        } else {
            exit(0);
        }
    } else if (strcmp(args[0], "cd") == 0) {
        if (args_num != 2) {
            printError(); // "cd" takes one argument
        } else {
            if (chdir(args[1]) != 0) {
                printError(); // Error if directory change fails
            }
        }
    } else if (strcmp(args[0], "path") == 0) {
        // Clear existing paths
        free(*paths);
        if (args_num == 1) {
            // If no arguments given, disable all paths
            *paths = NULL;
        } else {
            // Allocate memory for new paths array
            *paths = malloc(sizeof(char*) * args_num);
            (*paths)[args_num - 1] = NULL; // Null-terminate the paths array
            // Set new paths
            for (int i = 1; i < args_num; i++) {
                (*paths)[i - 1] = strdup(args[i]); // Duplicate and store each path
            }
        }
    } else {
        // Handling of external commands
        if (*paths == NULL || (*paths)[0] == NULL) {
            printError(); // If paths are not set, print an error
            return;
        }
        
        char *executablePath = findExecutable(args[0]);
        if (!executablePath) {
            printError(); // If command not found in any path, print an error
            return;
        }

        pid_t pid = fork();
        if (pid == 0) {
            // Child process attempts to execute the command
            execv(executablePath, args);
            printError(); // If execv returns, there was an error
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            // Fork failed
            printError();
        } else {
            // Parent process waits for child to complete
            waitpid(pid, NULL, 0);
        }
    }
}

int main(int argc, char *argv[]) {
    char *line = NULL;
    size_t bufsize = 0;
    ssize_t lineSize;
    char **default_paths = malloc(sizeof(char*));
    default_paths[0] = strdup("/bin");
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

        // Parse the input into an array of arguments
        while (part != NULL && args_num < MAX_ARGS) {
            args[args_num++] = part;
            part = strsep(&line, " ");
        }
        args[args_num] = NULL; // NULL-terminate the array

        // Execute the command
        executeCommands(args, args_num, &default_paths);
        
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
    for (int i = 0; default_paths && default_paths[i];i++)
    {
    free(default_paths[i]);        /* code */
    }
    
    return 0;
}
