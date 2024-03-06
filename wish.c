
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h>

int chng_directory(char *path){

    if(path == NULL || strcmp(path, "~") == 0){
        path = getenv("HOME");
}

    if (chdir(path)!=0){
        perror("cd");
        return -1;

    }
    return O;
}
