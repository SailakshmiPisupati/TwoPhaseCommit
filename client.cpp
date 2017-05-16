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
char* file_to_read;

int get_file_count(){
   FILE *get_records;
   int transaction_count=1;
   get_records =fopen("transaction.txt","r");
   if(!get_records){
      printf("Unable to open file\n");
   }
   //Get the total number of lines i.e. accounts in the file.
   int ch=0;
   while(!feof(get_records)){
      ch = fgetc(get_records);
      if(ch=='\n'){
         transaction_count++;
      }
   }
   fclose(get_records);
   return transaction_count;
}

int main(int argc, char *argv[])
{
  if(argc < 2){
    printf("Please provide the front end port number and file name.\n");
  }else{
    server_port = atoi(argv[1]);
    file_to_read = argv[2];
  }
  printf("Online Banking Application\n");
  printf("file_to_read %s\n", file_to_read);

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

  FILE *get_records; 
  get_records =fopen(file_to_read,"r");
  if(!get_records)
    printf("Unable to open file\n");
  while(fgets(message,2000,get_records)){
      printf("Message %s\n", message);
      int sendval = send(sock,&message,sizeof(message),0); 
      if(sendval<0)
       printf("Error in send");
      sleep(5);
      int recn = recv(sock, server_reply,2000,0);
      if(recn>0){
          printf("%s\n", server_reply);
      }else{
        printf("Error in receiving server_reply.\n");
      }
      sleep(5);
   }
}