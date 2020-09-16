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

double usage = 0;
double old_idle = 0;
double new_idle;
double average = 0;
double max = 0;//max usage
int cnt = 0;//count the number of seconds

extern pthread_mutex_t lock; //lock for threads
extern double frequency;// sleep frequency


void fun(){
    while(1){
        pthread_mutex_lock(&lock);// attempt to acquire the lock, wait here if another thread holds it
        double time = frequency / 1000;//change to seconds
        printf("sleep time: %f\n", time);
        pthread_mutex_unlock(&lock);
        sleep(time);
        FILE* file = NULL;
        file = fopen("/proc/stat", "r");
        char* line = malloc(101 * sizeof(char));
        fgets(line, 101, file);// get the cpu line
        line[strlen(line) - 1] = '\0';//change the \n to '\0'
        if(line == NULL) {
            printf("Error in reading line!\n");
            free(line);
            fclose(file);
            return;
        }
        
        const char s[10] = " "; // the sign to split the header
        char* token;//content of header    
        token = strtok(line, s);//split for the first time: "cpu"
        int count = 0;
        
        //loop and count the 4th element idle
        while(count < 4){
            token = strtok(NULL, s);
            count++;
        }
        new_idle = (double)atoi(token);
        cnt++;
        usage = 100 - (new_idle - old_idle) / 4; // usage
        if(usage < 0) usage = 0;
        else if(usage > 100) usage = 100;
        pthread_mutex_lock(&lock);// attempt to acquire the lock, wait here if another thread holds it
        //cnt++;//the counter increase by 1 for 1 s
        if(usage > max) max = usage;
        average = ((cnt - 1) * average + usage) / cnt;//update the average usage
        pthread_mutex_unlock(&lock);// unlock
        
        //printf("usage: %f\n", usage);
        old_idle = new_idle;
        free(line);
        fclose(file);
    }
}