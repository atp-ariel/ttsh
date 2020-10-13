#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv){
        int maxSize = 1024;
    int pos = 0;
    int* pos2 = calloc(1, sizeof(int));
    *pos2 = 0;
    char** tokens = calloc(1, maxSize*sizeof(char*));
    char* token = calloc(1, 1024 * sizeof(char));
    int quotation = 0;
    while(*argv != '\0'){
        token = calloc(1, 1024 * sizeof(char));
        pos = 0;
        if(*argv == '\"'){
            quotation++;
            argv++;
            while(*argv != '\"'){
                if(*argv == '\0'){
                    printf("ERROR\n");
                    return NULL;
                }
                token[pos++] = *argv;
                if(pos == sizeof(token) -1){
                    maxSize *= 2;
                    token = realloc(token, maxSize * sizeof(char));
                }
                argv++;
            }
            quotation++;
            argv++;
        }
        else if(!strncmp(argv, " ", 1)){
            argv++;
            continue;
        }
        else{
            while(strncmp(argv, " ", 1) && strncmp(argv, "\0", 1))
            {
                token[pos++] = *argv;
                argv++;
            }
        }
        if(*pos2 == sizeof(tokens)-1){
            maxSize *= 2;
            tokens = realloc(tokens, sizeof(char*) * maxSize);
        }
        tokens[*pos2] = token;
        *pos2 = *pos2 + 1;
    }
}