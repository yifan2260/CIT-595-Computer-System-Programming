/* 
This code primarily comes from 
http://www.prasannatech.net/2008/07/socket-programming-tutorial.html
and
http://www.binarii.com/files/papers/c_sockets.txt
 */

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


//global variable
double average_last_hour = 0;//average usage for last hour
clock_t start;
clock_t end;//count the time
char* html;
char* resp[2];
double frequency = 1000;// sleep frequency ms
int sock; // socket descriptor

pthread_t t1 ;
pthread_t t2 ;
pthread_t t3 ;
pthread_t thread[2];//2 threads for dealing with requests
pthread_mutex_t lock; //lock for threads
pthread_mutex_t locks[2]; //locks for threads3

extern void fun();
extern double usage;
extern double average;
extern double max;

extern char* readFile(char*);
void freeAll();

typedef struct Sserver{
    int sock;
    int fd;
    int sin_size;
    int num;
    struct sockaddr_in server_addr,client_addr;  
}sserver;
sserver s0, s1;
//sserver* head = NULL;

void* funct(void *p){
    
    // 4. accept: wait here until we get a connection on that port
    sserver* ss = (sserver*)p;
    while(1){
        ss-> sin_size = sizeof(struct sockaddr_in);
        ss->fd = accept(ss->sock, (struct sockaddr *)&(ss->client_addr),(socklen_t *)&(ss->sin_size));
		if (ss->fd != -1) {
			printf("Server got a connection from (%s, %d)\n", inet_ntoa((ss->client_addr).sin_addr),ntohs((ss->client_addr).sin_port));
            
			// buffer to read data into
			char request[2048];
            
			// 5. recv: read incoming message (request) into buffer
			int bytes_received = recv(ss->fd,request,2048,0);
            printf("Thread %d is working!\n\n", ss-> num);
            // null-terminate the string
			request[bytes_received] = '\0';
			// print it to standard out
			printf("REQUEST:\n%s\n", request);
			if(request[0] == 'P') {
                const char s[2] = "\"";
                char *token;
                token = strtok(request, s);
                double freq = 1000;
                while( token != NULL ) {
                    freq = (double)atoi(token);
                    if(freq > 0) break;
                    token = strtok(NULL, s);
                }
                pthread_mutex_lock(&lock);// attempt to acquire the lock, wait here if another thread holds it
                frequency = freq;
                pthread_mutex_unlock(&lock);// attempt to acquire the lock, wait here if another thread holds it
            }
            /*update average usage, timer, max usage and average usage for last hour*/
            end = clock();
            if(end / CLOCKS_PER_SEC < 3600 || (end - start)/CLOCKS_PER_SEC >= 3600){
                pthread_mutex_lock(&lock);// attempt to acquire the lock, wait here if another thread holds it
                average_last_hour = average;// time less than 1h or it is the time to update
                pthread_mutex_unlock(&lock);
                start = end;//renovate start to count for one hour
            }
            // this is the message that we'll send back
            resp[ss->num] = (char*)malloc(8192 * sizeof(char));
            pthread_mutex_lock(&lock);// attempt to acquire the lock, wait here if another thread holds it
			sprintf(resp[ss->num], "HTTP/1.1 200 OK\nContent-Type: application/json\n\n{\"usage\" : \"%f\",\"max\" : \"%f\", \"average\" : \"%f\"}",usage,max,average_last_hour);
            pthread_mutex_unlock(&lock);

			// 6. send: send the outgoing message (response) over the socket
			// note that the second argument is a char*, and the third is the number of chars	
			if(request[0] != 'P'){
                printf("RESPONSE:\n%s\n", resp[ss->num]);
                send(ss->fd, resp[ss->num], strlen(resp[ss->num]), 0);
            }

			free(resp[ss->num]);

			// 7. close: close the connection
			close(ss->fd);
			printf("Server closed connection\n");
       }
    }
    
}
int start_server(int PORT_NUMBER)
{

    // structs to represent the server and client
    struct sockaddr_in server_addr,client_addr;    

      // 1. socket: creates a socket descriptor that you later use to make other system calls
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Socket");
		exit(1);
    }
    int temp = 1;
    if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&temp,sizeof(int)) == -1) {
		perror("Setsockopt");
		exit(1);
    }

      // configure the server
    server_addr.sin_port = htons(PORT_NUMBER); // specify port number
    server_addr.sin_family = AF_INET;         
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    bzero(&(server_addr.sin_zero),8); 
      
      // 2. bind: use the socket and associate it with the port number
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
		perror("Unable to bind");
		exit(1);
    }

      // 3. listen: indicates that we want to listen to the port to which we bound; second arg is number of allowed connections
    if (listen(sock, 1) == -1) {
		perror("Listen");
		exit(1);
    }
          
      // once you get here, the server is set up and about to start listening
      printf("\nServer configured to listen on port %d\n", PORT_NUMBER);
      fflush(stdout);
    
    /*Initial request from the client*/
    int sin_size = sizeof(struct sockaddr_in);
    int fd = accept(sock, (struct sockaddr *)&client_addr,(socklen_t *)&sin_size);
    html = malloc(8192 * sizeof(char));
    strcpy(html, "HTTP/1.1 200 OK\nContent-Type: text/html\n\n");
    char* tmp = readFile("auto_update.html");
    strcat(html, tmp);
    free(tmp);
        if (fd != -1) {
			printf("Server got a connection from (%s, %d)\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

			char request[1024];

			int bytes_received = recv(fd,request,1024,0);
			// null-terminate the string
			request[bytes_received] = '\0';
			// print it to standard out
			printf("REQUEST:\n%s\n", request);

			char* response = (char*)malloc(8192 * sizeof(char));
			
            /*update average usage, timer, max usage and average usage for last hour*/            
            end = clock();
            if(end / CLOCKS_PER_SEC < 3600 || (end - start)/CLOCKS_PER_SEC >= 3600){
                pthread_mutex_lock(&lock);// attempt to acquire the lock, wait here if another thread holds it
                average_last_hour = average;// time less than 1h or it is the time to update
                pthread_mutex_unlock(&lock);
                start = end;//renovate start to count for one hour
            }
            pthread_mutex_lock(&lock);// attempt to acquire the lock, wait here if another thread holds it
			sprintf(response, html, usage, max, average_last_hour);
            pthread_mutex_unlock(&lock);
            printf("RESPONSE:\n%s\n", response);

			// 6. send: send the outgoing message (response) over the socket
			// note that the second argument is a char*, and the third is the number of chars	
			send(fd, response, strlen(response), 0);
			free(response);

			// 7. close: close the connection
			close(fd);
			printf("Server closed connection\n");
       }
    free(html);
    
    for(int I = 0; I < 2; I++){  
        sserver* ss;
        if(I == 0) ss = &s0;
        else ss = &s1;
        ss -> sin_size = sin_size;
        ss -> sock = sock;
        ss -> num = I;
        ss -> fd = fd;
        ss -> client_addr = client_addr;
        //ss -> next = head;
        //head = ss;
        int ret = pthread_create(&thread[I], NULL, funct, (void*)(ss));
        if(ret!=0){
             printf("Error: thread create failed\n");
             return 0;
         }
    }
    
    for(int I = 0; I < 2; I++){
        int ret = pthread_join(thread[I], NULL);
         if(ret!=0){
             printf("Error: thread join failed");
             return 0;
         }
    }
    // 8. close: close the socket
    close(sock);
    printf("Server closed connection\n");
  
    return 0;
} 




/* This function is for call start_server */
void* fun1(void *p){
    int PORT_NUMBER = *(int*) p;
    start_server(PORT_NUMBER);
    return NULL;
}

/* This function is for update cpu usage */

void* fun2(void *p){
    fun();
    return NULL;
}

/* This function wait for the user to end the program*/
void* fun3(void *p){
    while(1){
        //printf("Enter the letter 'q' and hit Enter/Return to stop the program\n");
        char* input = malloc(101 * sizeof(char));
        scanf("%s", input);
        if(strcmp(input, "q") == 0){
            printf("Server closed connection\n");
			printf("Shutting down\n");            
            free(input);
            exit(0);
        }
        free(input);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    /*
  // check the number of arguments
  if (argc != 2) {
      printf("\nUsage: %s [port_number]\n", argv[0]);
      exit(-1);
  }

  int port_number = atoi(argv[1]);
  if (port_number <= 1024) {
    printf("\nPlease specify a port number greater than 1024\n");
    exit(-1);
  }*/
	start = clock();
	pthread_mutex_init(&lock, NULL);// initialize the lock
    pthread_mutex_init(&locks[0], NULL);// initialize the lock
    pthread_mutex_init(&locks[1], NULL);// initialize the lock
    int port_number = 3000; // hard-coded for use on Codio

    int ret = pthread_create (&t1, NULL, fun1, &port_number) ;
    if (ret != 0) return 1;
    
    ret = pthread_create(&t2, NULL, fun2, NULL);
	if (ret != 0) return 1;
    
    ret = pthread_create(&t3, NULL, fun3, NULL);
	if (ret != 0) return 1;
    
    ret = pthread_join(t1, NULL);
    if (ret != 0) return 1;
    
    ret = pthread_join(t2, NULL) ;
    if (ret != 0) return 1;
    
    ret = pthread_join(t3, NULL) ;
    if (ret != 0) return 1;
    
    return 0;
}

/*
void freeAll(){
    sserver* ss = head;
    while(ss != NULL){
        sserver* n = ss;
        ss = ss -> next;
        close(n -> fd);
        free(n);
    }
    head = NULL;   
}*/

