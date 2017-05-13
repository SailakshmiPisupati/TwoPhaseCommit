#include <stdio.h>
#include <string.h>    
#include <stdlib.h>    
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>   
#include <pthread.h>
#include <stdbool.h>
#include <time.h>

int get_request_type();
int get_random();
void* create_server_connection(void* args);

int server_port;

int main(int argc, char const *argv[])
{
  if(argc < 1){
    printf("Please provide the front end port number.\n");
  }else{
    server_port = atoi(argv[1]);
  }

  pthread_t client_communication;

  int n = pthread_create( &client_communication , NULL ,  create_server_connection , NULL);      
  if( n  < 0){
    perror("Could not create receiver thread");
  }
  pthread_join(client_communication, NULL);
  
 // create_server_connection();
  return 0;
}

int get_request_type(){
  // types of request - 0- CREATE, 1- UPDATE, 2- QUERY, 3- QUIT
  int type_of_request = 1;
  type_of_request = get_random();
  return type_of_request;
}

int get_random(){
  srand(time(NULL));
  int max = 3, min = 0;
  int random = (rand() % (max + 1)-min) + min;
  return random;
}

void* create_server_connection(void* args){
  int sock;
  struct sockaddr_in server;
  char message[20] , server_reply[2000];
  sock = socket(AF_INET , SOCK_STREAM , 0);
  if (sock == -1)
  {
    printf("Could not create socket");
  }
   
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_family = AF_INET;
  server.sin_port = htons(server_port);

  //Connect to remote server
  if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
  {
    perror("connect failed. Error");
  }

  int request_type = get_request_type();
  char type[10];
  printf("Requesting for process %d\n",request_type);
  if(request_type == 0){
    int amount = get_random();
    printf("CREATE %d\n",amount);
    snprintf(type, sizeof(type), "%s", "CREATE");
    snprintf(message, sizeof(message), "%s %d", request_type, amount);
    int sendval = send(sock,&message,sizeof(message),0); 
    if(sendval<0)
        printf("Error in send");
  }else if(request_type == 1){
    int amount = get_random();
    int account = get_random();
    printf("UPDATE %d %d\n",account, amount);
    snprintf(type, sizeof(type), "%s", "UPDATE");
    snprintf(message, sizeof(message), "%s %d %d", request_type, account, amount);
    int sendval = send(sock,&message,sizeof(message),0); 
    if(sendval<0)
        printf("Error in send");
  }else if(request_type == 2){
    printf("QUERYING\n");
    int account = get_random();
    printf("QUERY %d\n",account);
    snprintf(type, sizeof(type), "%s", "QUERY");
    snprintf(message, sizeof(message), "%s %d", request_type, account);
    int sendval = send(sock,&message,sizeof(message),0); 
    if(sendval<0)
        printf("Error in send");
  }else if(request_type == 3){
    printf("QUIT\n");
    
  }
}