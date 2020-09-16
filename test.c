#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

char* readFile(char* filename){
    FILE *fp;
    fp = fopen(filename , "r");
    fseek( fp , 0 , SEEK_END );
    int file_size;
    file_size = ftell( fp );
    printf( "%d\n" , file_size );
    char *tmp;
    fseek( fp , 0 , SEEK_SET);
    tmp =  (char *)malloc( file_size * sizeof( char ) );
    fread( tmp , file_size , sizeof(char) , fp);
    //printf("%s\n" , tmp );
    return tmp;
}