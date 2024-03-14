#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFSIZE 4096

void err_sys(const char* msg){
    printf("%s", msg);
}

int main(void){
    int n;
    char buf[BUFFSIZE];

    while((n = read(STDIN_FILENO, buf, BUFFSIZE)) > 0){
        if(write(STDOUT_FILENO, buf, n) != n){
            printf("write error");
            exit(1);
        }
    }
    
    if(n < 0){
        err_sys("read error");
        exit(1);
    }
    
    exit(0);
}
