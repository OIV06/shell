#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <pthread.h> 
// Define constants for mode and buffer size.
#define INTERACTIVE_MODE 1
#define BATCH_MODE 2
#define BUFF_SIZE 1024

// Declare global variables.
int pathNULL = 0;
char *paths[BUFF_SIZE] = {"/bin", NULL};
char *line = NULL;
FILE *in = NULL;
struct function_args {
  pthread_t thread;
  char *command;
};
// Function declarations.
void printError() {
    const char error_message[] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}
void clean(void) {
  if (line) {
    free(line);
  }
  if (in) {
    fclose(in);
  }
}
char *trim(char *str) {
  while (isspace((unsigned char)*str)) str++;
  if (*str == 0)  // All spaces?
    return str;

  char *end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char)*end)) end--;
  end[1] = '\0';

  return str;
}
int searchPath(char *firstArg) {
  static char path[BUFF_SIZE];
  if (pathNULL) {
    return -1; // Don't search for executables if pathNULL is set.
  }
  for (int i = 0; paths[i] != NULL; i++) {
    snprintf(path, BUFF_SIZE, "%s/%s", paths[i], firstArg);
    if (access(path, X_OK) == 0)
      return 0;
  }
  return -1; // Executable not found in any listed paths.
}
void redirect(FILE *out) {
  fflush(stdout);
  int outFileno = fileno(out);
  if (outFileno == -1) {
    printError();
    return;
  }
  if (outFileno != STDOUT_FILENO) {
    if (dup2(outFileno, STDOUT_FILENO) == -1) {
      printError();
      return;
    }
    if (out != stderr && out != stdout) {
      fclose(out);
    }
  }
}

void executeCommands(char *args[], int args_num, FILE *out) {
  // Built-in command: 'exit'
  if (strcmp(args[0], "exit") == 0) {
    if (args_num > 1) {
      printError();
    } else {
      clean();
      exit(EXIT_SUCCESS);
    }
  }
  // Built-in command: 'cd'
  else if (strcmp(args[0], "cd") == 0) {
    if (args_num != 2 || chdir(args[1]) != 0) {
      printError();
    }
  }
  // Built-in command: 'path'
  else if (strcmp(args[0], "path") == 0) {
    for (size_t i = 0; i < BUFF_SIZE; i++) {
      free(paths[i]);
      paths[i] = (i + 1 < args_num) ? strdup(args[i + 1]) : NULL;
    }
  }
  // Built-in command: 'restrict'
  else if (strcmp(args[0], "restrict") == 0) {
    if (args_num != 2 || (strcmp(args[1], "0") != 0 && strcmp(args[1], "1") != 0)) {
      printError();
    } else {
      pathNULL = atoi(args[1]);
    }
  }
  // External commands
  else {
    if (searchPath(args[0]) == -1) {
      printError();
      return;
    }
    pid_t pid = fork();
    if (pid == 0) {
      // Child process
      redirect(out);
      char *envp[] = {NULL};
      if (execve(searchPath, args, envp) == -1) {
        printError();
        exit(EXIT_FAILURE);
      }
    } else if (pid < 0) {
      // Error forking
      printError();
    } else {
      // Parent process
      waitpid(pid, NULL, 0);
    }
  }
}

// Include all the previously defined functions here, like printError, clean, trim, redirect, searchPath, etc.

int main(int argc, char *argv[]) {
    int mode = INTERACTIVE_MODE;
    size_t linecap = 0;

    // Batch mode check.
    if (argc > 1) {
        mode = BATCH_MODE;
        in = fopen(argv[1], "r");
        if (!in) {
            printError();
            exit(EXIT_FAILURE);
        }
    } else {
        in = stdin;
    }

    while (1) {
        if (mode == INTERACTIVE_MODE) {
            printf("wish> ");
            fflush(stdout);
        }

        if (getline(&line, &linecap, in) == -1) {
            if (feof(in)) {
                // End of file reached or error occurred.
                break;
            } else {
                // Handle error.
                printError();
                continue;
            }
        }

        // Trim newline character.
        line[strcspn(line, "\n")] = 0;
        
        // Tokenize the input line into arguments.
        char *args[BUFF_SIZE];
        char *part = strtok(line, " ");
        int args_num = 0;
        while (part != NULL && args_num < BUFF_SIZE) {
            args[args_num++] = strdup(trim(part)); // Allocate new string for argument
            part = strtok(NULL, " ");
        }
        args[args_num] = NULL;

        // Execute the commands.
        FILE *out = stdout; // Default output file stream.
        executeCommands(args, args_num, out);

        // Free the allocated arguments.
        for (int i = 0; i < args_num; i++) {
            free(args[i]);
        }

        // Reset the line buffer for the next read.
        free(line);
        line = NULL;
    }

    // Clean up before exiting.
    if (in != stdin) {
        fclose(in);
    }
    free(line);
    
    return 0;
}

