include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
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
char *default_paths[MAX_PATHS] = {"/bin", NULL}; // Default search path with room for expansion.

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
    default_paths[i] = NULL; // Terminate the new path list.
}

char *findExecutable(char *command)
{
    static char path[PATH_LEN];
    if (command[0] == '/' || command[0] == '.')
    {
        // If it's an absolute or relative path, return it directly.
        if (access(command, X_OK) == 0)
        {
            return command;
        }
        return NULL; // Not executable or not found
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
    return NULL; // Executable not found in any search path.
}
void redirect(int outFileno)
{
    if (outFileno != STDOUT_FILENO)
    {
        // Redirect stdout
        if (dup2(outFileno, STDOUT_FILENO) == -1)
        {
            printError();
            exit(EXIT_FAILURE);
        }
        // Redirect stderr
        if (dup2(outFileno, STDERR_FILENO) == -1)
        {
            printError();
            exit(EXIT_FAILURE);
        }
        close(outFileno);
    }
}
void executeCommands(char *args[], int args_num)
{
    if (strcmp(args[0], "exit") == 0)
    {
        if (args_num > 1)
        {
            printError(); // "exit" takes no arguments.
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
            printError(); // "restrict" takes no arguments.
        }
        else
        {
            pathNULL = !pathNULL; // Toggle restriction mode.
            printf("Command execution is now %s.\n", pathNULL ? "restricted" : "unrestricted");
        }
    }
    else if (strcmp(args[0], "path") == 0)
    {
        updatePaths(&args[1], args_num - 1); // Update the search paths.
    }
    else if (!pathNULL)
    {
        char *executablePath = findExecutable(args[0]);
        if (!executablePath)
        {
            printError();
            return;
        }

        int redirectIndex = -1;
        char *filename = NULL;
        for (int i = 0; i < args_num; i++)
        {
            if (strcmp(args[i], ">") == 0)
            {
                if (redirectIndex != -1)
                {
                    // Multiple redirection operators found
                    printError();
                    return;
                }
                redirectIndex = i;
            }
        }

        if (redirectIndex != -1)
        {
            // Check redirection validity
            if (redirectIndex == args_num - 1 || args_num - redirectIndex > 2)
            {
                // No filename specified or too many arguments after '>'
                printError();
                return;
            }
            filename = args[redirectIndex + 1];
            args[redirectIndex] = NULL; // Terminate command arguments before '>'
        }

        // Handle built-in commands (exit, cd, path)

        // Non built-in command execution
        if (!pathNULL && args[0] != NULL)
        {
            char *executablePath = findExecutable(args[0]);
            if (!executablePath)
            {
                printError();
                return;
            }

            pid_t pid = fork();
            if (pid == -1)
            {
                // Fork failed
                printError();
            }
            else if (pid == 0)
            {
                // In the child process

                // Handle redirection if needed
                if (filename != NULL)
                {
                    int outFileno = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (outFileno == -1)
                    {
                        printError();
                        exit(EXIT_FAILURE);
                    }
                    if (dup2(outFileno, STDOUT_FILENO) == -1 || dup2(outFileno, STDERR_FILENO) == -1)
                    {
                        printError();
                        close(outFileno);
                        exit(EXIT_FAILURE);
                    }
                    close(outFileno);
                }

                // Execute the command
                execv(executablePath, args);
                // If execv returns, it's an error
                printError();
                exit(EXIT_FAILURE);
            }
            else
            {
                // In the parent process
                waitpid(pid, NULL, 0);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    char *line = NULL;
    size_t bufsize = 0;
    ssize_t lineSize;

    FILE *input_stream = (argc == 1) ? stdin : fopen(argv[1], "r");
    if (input_stream == NULL)
    {
        fprintf(stderr, "wish: cannot open file\n");
        return 1;
    }

    while ((lineSize = getline(&line, &bufsize, input_stream)) != -1)
    {
        if (line[lineSize - 1] == '\n')
        {
            line[lineSize - 1] = '\0';
        }

        char *args[MAX_ARGS];
        int args_num = 0;
        char *part = strsep(&line, " ");
        while (part != NULL && args_num < MAX_ARGS)
        {
            args[args_num++] = part;
            part = strsep(&line, " ");
        }
        args[args_num] = NULL; // NULL-terminate the argument list

        executeCommands(args, args_num);

        if (argc == 1)
        {
            printf("wish> ");
        }
    }

    free(line);
    if (argc > 1)
    {
        fclose(input_stream);
    }
    return 0; // Exit gracefully on EOF or closing file
}
