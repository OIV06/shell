#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
void printError();
void updatePaths(char *newPaths[], int numPaths);
char *findExecutable(char *command);
void executeCommands(char *args[], int args_num);
void executeExternalCommand(char *args[], int args_num);
void executeParallelCommands(char *line);
#define MAX_LINE 1024 // The maximum length command
#define MAX_ARGS 64
#define PATH_LEN 1024
#define MAX_PATHS 16
FILE *in = NULL;
char *line = NULL;
int pathNULL = 0;
void clean(void)
{
    free(line);
    fclose(in);
}
char *default_paths[MAX_PATHS] = {"/bin", NULL}; 

void printError()
{
    const char error_message[] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void updatePaths(char *newPaths[], int numPaths)
{
    int i;
    for (i = 0; i < numPaths; i++)
    {
        default_paths[i] = strdup(newPaths[i]);
    }
    default_paths[i] = NULL;
}

char *tokenize(char *input) {
    int i;
    int j = 0;
    char *tokenized = (char *)malloc((MAX_LINE * 2) * sizeof(char));
    // add spaces around special characters
    for (i = 0; input[i] != '\0'; i++) {
        if (input[i] == '>' || input[i] == '<' || input[i] == '|'||input[i] == '&') {
            tokenized[j++] = ' ';
            tokenized[j++] = input[i];
            tokenized[j++] = ' ';
        } else {
            tokenized[j++] = input[i];
        }
    }
    tokenized[j] = '\0';
    return tokenized;
}


char *findExecutable(char *command)
{
    static char path[PATH_LEN];
    if (command[0] == '/' || command[0] == '.')
    {
        
        if (access(command, X_OK) == 0)
        {
            return command;
        }
        return NULL; 
    }
    else
    {
        for (int i = 0; default_paths[i] != NULL; i++)
        {
            snprintf(path, PATH_LEN, "%s/%s", default_paths[i], command);
            if (access(path, X_OK) == 0)
            {
                return path;
            }
        }
    }
    return NULL;
}
void redirect(int outFileno)
{
    if (outFileno != STDOUT_FILENO)
    {
        
        if (dup2(outFileno, STDOUT_FILENO) == -1)
        {
            printError();
            exit(EXIT_FAILURE);
        }
       
        if (dup2(outFileno, STDERR_FILENO) == -1)
        {
            printError();
            exit(EXIT_FAILURE);
        }
        close(outFileno);
    }
}
void executeCommands(char *args[], int args_num)
{   if (args_num == 0){
    return;
}
    if (strcmp(args[0], "exit") == 0)
    {
        if (args_num > 1)
        {
            printError(); 
        }
        else
        {
            exit(0);
        }
    }
    else if (strcmp(args[0], "cd") == 0)
    {
        if (args_num != 2)
        {
            printError();
        }
        else
        {
            if (chdir(args[1]) != 0)
            {
                printError();
            }
        }
    }
    else if (strcmp(args[0], "restrict") == 0)
    {
        if (args_num > 1)
        {
            printError(); 
        }
        else
        {
            pathNULL = !pathNULL; 
            printf("Command execution is now %s.\n", pathNULL ? "restricted" : "unrestricted");
        }
    }
    else if (strcmp(args[0], "path") == 0)
    {
        updatePaths(&args[1], args_num - 1); 
    }
    else {
        if (!pathNULL){
            executeExternalCommand(args, args_num);
        }
    }
}
 void executeExternalCommand(char *args[], int args_num){ 
        char *executablePath = findExecutable(args[0]);
    if (!executablePath) {
        printError();
        return;
    }

    // Find redirection index, if any
    int redirectIndex = -1;
    char *filename = NULL;
    for (int i = 0; i < args_num; i++) {
        if (strcmp(args[i], ">") == 0) {
            if (redirectIndex != -1) {
                printError();
                return;
            }
            redirectIndex = i;
            break; 
        }
    }

    
    if (redirectIndex != -1) {
        if (redirectIndex == args_num - 1 || args_num - redirectIndex > 2) {
            // No filename specified or too many arguments after '>'
            printError();
            return;
        }
        filename = args[redirectIndex + 1];
        args[redirectIndex] = NULL; 
    }

    
    pid_t pid = fork();
    if (pid == -1) {
        
        printError();
        return;
    } else if (pid == 0) {
        
        if (filename != NULL) {
            int outFileno = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (outFileno == -1) {
                printError();
                exit(EXIT_FAILURE);
            }
            if (dup2(outFileno, STDOUT_FILENO) == -1 || dup2(outFileno, STDERR_FILENO) == -1) {
                printError();
                close(outFileno);
                exit(EXIT_FAILURE);
            }
            close(outFileno);
        }

        // Execute the command
        execv(executablePath, args);
        
        printError();
        exit(EXIT_FAILURE);
    } else {
        // In the parent process
        waitpid(pid, NULL, 0);
    }
}
void executeParallelCommands(char *line) {
    char *commands[MAX_ARGS]; // Assuming a maximum number of parallel commands
    int nCommands= 0;
    char *command = strtok(line, "&");
    while (command != NULL && nCommands < MAX_ARGS) {
        commands[nCommands++] = command;
        command = strtok(NULL, "&");
    }

    pid_t pids[nCommands]; // To store child PIDs

    for (int i = 0; i < nCommands; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            // Child process
            char *args[MAX_ARGS];
            int args_num = 0;
            char *token = strtok(commands[i], " ");
            while (token != NULL && args_num < MAX_ARGS) {
                args[args_num++] = token;
                token = strtok(NULL, " ");
            }
            args[args_num] = NULL;
            executeCommands(args, args_num);
            exit(0); // Ensure child exits after execution
        } else if (pids[i] < 0) {
            // Forking failed
            printError();
        }
    }

    // Parent process: wait for all child processes to finish
    for (int i = 0; i < nCommands; i++) {
        if (pids[i] > 0) {
            waitpid(pids[i], NULL, 0);
        }
    }
}

int main(int argc, char *argv[])
{
    size_t bufsize = 0;
    ssize_t lineSize;
    FILE *input_stream = stdin;  // Default input stream is stdin for interactive mode.
    char *args[MAX_ARGS];
    char *line = NULL;

    // Handling invalid usage scenario: more than one input file for batch mode.
    if (argc > 2)
    {
        printError();
        exit(EXIT_FAILURE);
    }
    else if (argc == 2)
    {
        // Batch mode: attempt to open the specified file.
        input_stream = fopen(argv[1], "r");
        if (!input_stream)
        {
            // If the file cannot be opened, print an error message and exit.
            printError();
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        // Interactive mode: Display the prompt.
        printf("wish> ");
        fflush(stdout);
    }

    while ((lineSize = getline(&line, &bufsize, input_stream)) != -1)
    {
        if (line[lineSize - 1] == '\n')
        {
            line[lineSize - 1] = '\0';  // Remove newline character.
        }
          if (strchr(line, '&') != NULL) {
            // Handle parallel commands
            executeParallelCommands(line); // Implement this function
        } else {

        char *preprocessedLine = tokenize(line);  // Tokenize the input line.
        int args_num = 0;
        char *token = strtok(preprocessedLine, " ");
        while (token != NULL && args_num < MAX_ARGS)
        {
            args[args_num++] = token;  // Collect arguments.
            token = strtok(NULL, " ");
        }
        args[args_num] = NULL;  // NULL-terminate the argument list.

         executeCommands(args, args_num);

        if (argc == 1)
        {
            // If in interactive mode, print prompt again after command execution.
            printf("wish> ");
            fflush(stdout);
        }
        free(preprocessedLine);
    }
    
    }

    free(line);  // Clean up the allocated line buffer.
    if (argc > 1)
    {
        fclose(input_stream);  // Close the file stream if in batch mode.
    }
    return 0;  // Exit gracefully.
}
