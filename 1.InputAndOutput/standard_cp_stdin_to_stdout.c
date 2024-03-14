#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFSIZE 4096

void err_sys(const char* msg){
    printf("%s", msg);
}

int main(void){
    int c;

    while( (c = getc(stdin)) != EOF ){
        if( putc(c,stdout) == EOF ){
            err_sys("output error");
        }
    }
    
    if(ferror(stdin)){
        err_sys("input error");
    }
    
    exit(0);
}
