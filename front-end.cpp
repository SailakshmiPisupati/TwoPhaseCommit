#include <stdio.h>
#include <string.h>    
#include <stdlib.h>    
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>   
#include <pthread.h>
#include <stdbool.h>
#include <time.h>

int node_ports[4];
int front_end_port;
int last_created_account = 0;
int accounts[1000];
int number_of_nodes=3;

struct{
    int account_no;
    int account_status;
    pthread_mutex_t lock_account;
}

void front_end_processor();
void* receivemessages(void* arg);

int main(int argc, char const *argv[])
{
	front_end_port = atoi(argv[1]);
    node_ports[0] = atoi(argv[2]);
    node_ports[1] = atoi(argv[3]);
    node_ports[2] = atoi(argv[4]);

    //read the input file
    front_end_processor();
	return 0;
}

void front_end_processor(){

	printf("A thread to receive connections from other data servers.\n");
    pthread_t client_thread;
    int n = pthread_create( &client_thread , NULL ,  receivemessages , NULL);      
    if( n  < 0){
      perror("Could not create receiver thread");
    }
    pthread_join(client_thread, NULL);
}

void* receivemessages(void* arg){
	int socket_desc , c , *new_sock;
    int client_sock;
    struct sockaddr_in server , client;
    char message[2000];

    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket\n");
    }
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(front_end_port);
    printf("Front end port is %d\n", front_end_port);
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
        //print the error message
        perror("bind failed. Error");
    }
    printf("bind done\n");
    
    //Listen
    listen(socket_desc , 50);
    while(1){     
        //Accept and incoming connection
        // printf("Waiting for communication with the other nodes.\n");
        c = sizeof(struct sockaddr_in);
        client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        if (client_sock < 0){
            perror("accept failed");
        }

        int recn = recv(client_sock, message,2000,0);
        if(recn > 0){
            printf("client message is %s\n",message);
        }

        pthread_t connect_to_servers;
        int n = pthread_create( &connect_to_servers , NULL , init_two_phase_commit , message);      
        if( n  < 0){
          perror("Could not create receiver thread");
        }
        
    }
    close(client_sock);
    //close(socket_desc); 
}  

void *init_two_phase_commit(void* message){
    //connect to the other two servers and initiate 2 phase commit
    int sock;
    struct sockaddr_in server;
    printf("Connecting to other servers to initiate 2-phase commit.\n");
    
    
    for(int i=0;i<number_of_nodes;i++){
        sock = socket(AF_INET , SOCK_STREAM , 0);
        if(sock == -1){
        printf("Could not create socket");
        }

        server.sin_addr.s_addr = inet_addr("127.0.0.1");
        server.sin_family = AF_INET;

        server.sin_port = htons(node_ports[i]);

        //Connect to remote server
        if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0){
        perror("connect failed. Error");
        }

        snprintf(type, sizeof(type), "%s", "CREATE");
        int sendval = send(sock,&message,sizeof(message),0); 
        if(sendval<0)
            printf("Error in send");

    }
}