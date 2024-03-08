#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define MAX_ARGS 64

void printError() {
    const char error_message[] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void changeDirectory(char *path) {
    int rc = chdir(path);
    if (rc != 0) {
        
        printError();
    }
}
void executeCommands(char *args[], int args_num, FILE *out) {
   
    if (strcmp(args[0], "cd") == 0) {
        // Handling for "cd" command
        if (args_num != 2) {
            printError(); 
            return;
        } else {
            changeDirectory(args[1]); 
            return;
        }
    }
    // Other command handling would go here...
}





int main(int argc, char *argv[]) {
    char *line = NULL;
    size_t bufsize = 0;
    ssize_t lineSize;

    // Interactive mode prompt
    if (argc == 1) {
        printf("wish> ");
    }

    FILE *input_stream = (argc == 1) ? stdin : fopen(argv[1], "r");

    if (input_stream == NULL) {
        fprintf(stderr, "wish: cannot open file\n");
        return 1;
    }

    while ((lineSize = getline(&line, &bufsize, input_stream)) != -1) {
        // Remove the newline character at the end of the line
        if (line[lineSize - 1] == '\n') {
            line[lineSize - 1] = '\0';
        }

        // Parse the input into commands and arguments
        char *args[MAX_ARGS];
        int args_num = 0;
        char *token = strtok(line, " ");
        while (token != NULL && args_num < MAX_ARGS) {
            args[args_num++] = token;
            token = strtok(NULL, " ");
        }
        args[args_num] = NULL; // Argument list must be NULL terminated

        // Execute the parsed command
        executeCommands(args, args_num, stdout);

        // Print the prompt again if in interactive mode
        if (argc == 1) {
            printf("wish> ");
        }
    }

    // Cleanup and exit
    free(line);
    if (argc > 1) {
        fclose(input_stream);
    }
    return 0;
}
