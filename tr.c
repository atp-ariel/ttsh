#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

int main(){
    signal(SIGINT, SIG_IGN);

    for(int i= 0 ; i < 10; i++)
    {
        printf("%i\n", i);
        sleep(1);
    }
}