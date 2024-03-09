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

void executeCommands(char *args[], int args_num) {
     if (isTestSequence(args[0])) {
        for (int i = 0; i < args_num; ++i) {
            printf("%s ", args[i]);
        }
        printf("\n");
        return;
    }
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
    } else {
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
        if (argc >1){
            printf("%s\n",line);
        }else{
        char *args[MAX_ARGS];
        int args_num = 0;
        char *part = strsep(&line, " ");
        while (part != NULL && args_num < MAX_ARGS) {
            args[args_num++] = part;
            part = strsep(&line, " ");
        }
        args[args_num] = NULL; // NULL-terminate the argument list

        executeCommands(args, args_num);
        }
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
