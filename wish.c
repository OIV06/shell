#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#define MAX_ARGS 64
#define PATH_LEN 1024

// Global variable to control execution of non-built-in commands.
int pathNULL = 0; // 0 = unrestricted, 1 = restricted to built-in commands only.

void printError() {
    const char error_message[] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

char *default_paths[] = {"/bin", NULL}; // Default search path.

char* findExecutable(char *command) {
    static char path[PATH_LEN];
    if (command[0] == '/' || command[0] == '.') {
        return command;
    } else {
        for (int i = 0; default_paths[i] != NULL; i++) {
            snprintf(path, PATH_LEN, "%s/%s", default_paths[i], command);
            if (access(path, X_OK) == 0) {
                return path;
            }
        }
    }
    return NULL; // Executable not found
}

void executeCommands(char *args[], int args_num) {
    if (strcmp(args[0], "exit") == 0) {
        if (args_num > 1) {
            printError(); // "exit" takes no arguments.
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
    } else if (strcmp(args[0], "restrict") == 0) {
        if (args_num > 1) {
            printError(); // "restrict" takes no arguments.
        } else {
            pathNULL = !pathNULL; // Toggle restriction mode.
            printf("Command execution is now %s.\n", pathNULL ? "restricted" : "unrestricted");
        }
    } else if (!pathNULL) {
        char *executablePath = findExecutable(args[0]);
        if (!executablePath) {
            printError();
            return;
        }

        pid_t pid = fork();
        if (pid == 0) {
            if (execv(executablePath, args) == -1) {
                printError();
                exit(EXIT_FAILURE);
            }
        } else if (pid < 0) {
            printError();
        } else {
            waitpid(pid, NULL, 0);
        }
    } else {
        printError(); // Attempt to run non-built-in command while restricted.
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

    return 0;
}
