#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFF_SIZE 512 // Adjust as needed

// Placeholder for path initialization
char *paths[BUFF_SIZE] = {"/bin", NULL};

// Function to print errors as specified
void printError() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message)); 
}

// Function to trim leading and trailing whitespace
char *trim(char *str) {
    char *end;

    // Trim leading space
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0) { // All spaces?
        return str;
    }

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end+1) = 0;

    return str;
}

// Basic parsing and execution placeholder
void parse_and_execute(char *input) {
    input = trim(input); // Trim the input to remove leading and trailing spaces

    if(strcmp(input, "exit") == 0) { // Example built-in command
        exit(0); // Exit the shell
    }
    
    // Further parsing and execution logic will be added here
    // For external commands, use fork(), execv(), and waitpid()
}

int main(int argc, char *argv[]) {
    char *input = NULL;
    size_t bufsize = 0; // getline allocates buffer
    ssize_t nread;

    // Batch mode vs. Interactive mode
    if(argc == 1) { // Interactive mode
        while(1) {
            printf("wish> ");
            nread = getline(&input, &bufsize, stdin);
            if(nread == -1) {
                free(input);
                exit(0); // EOF or error
            }

            // Remove newline character
            if(input[nread - 1] == '\n') {
                input[nread - 1] = 0;
            }

            parse_and_execute(input);
        }
    } else if(argc == 2) { // Batch mode
        FILE *batch_file = fopen(argv[1], "r");
        if(!batch_file) {
            printError();
            exit(1);
        }

        while((nread = getline(&input, &bufsize, batch_file)) != -1) {
            // Remove newline character
            if(input[nread - 1] == '\n') {
                input[nread - 1] = 0;
            }

            parse_and_execute(input);
        }
        fclose(batch_file);
    } else {
        printError();
        exit(1);
    }

    free(input);
    return 0;
}

