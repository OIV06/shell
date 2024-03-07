#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define MAX_ARGS 64


void executeCommands(char *args[], int args_num, FILE *out) {
    // Check for built-in commands first
    if (strcmp(args[0], "cd") == 0) {
        // Handling for "cd" command
        if (args_num != 2) {
            printError(); // Incorrect number of arguments for cd
            return;
        } else if (chdir(args[1]) == -1) {
            printError(); // Error changing directory
            return;
        }
    }
    // Other command handling would go here...
}

void printError() {
    const char error_message[] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}



int main(int argc, char *argv[]) {
    // Setup for reading input and parsing commands
    char *line = NULL;
    size_t bufsize = 0;
    ssize_t lineSize;

    // Example setup for input loop
    while ((lineSize = getline(&line, &bufsize, stdin)) > 0) {
        // Parsing the input into commands and arguments
        char *args[MAX_ARGS]; 
        int args_num = 0;
        // Parsing logic here...

        // Execute the parsed command
        executeCommands(args, args_num, stdout);
        // Remember to free or clear any allocated memory as needed
    }

    // Cleanup and exit
    free(line);
    return 0;
}

