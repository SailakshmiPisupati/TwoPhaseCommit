#include <stdio.h>
#include <string.h>    
#include <stdlib.h>    
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>   
#include <pthread.h>
#include <stdbool.h>
#include <time.h>

int node_ports[3];
int front_end_port;
int last_created_account = 0;
int accounts[1000];
int number_of_nodes=3;
int abortop =0;
pthread_mutex_t lock_two_phase;
char fname[200];

void front_end_processor();
void* receivemessages(void* arg);
int send_to_all_servers(char* message, int client_sock);
int get_data(int connect_port, char * message);
int get_random_server();
int perform_two_phase_commit();
void * new_client_thread(void *client_sock);
FILE * filename;

int main(int argc, char *argv[])
{
	front_end_port = atoi(argv[1]);
    node_ports[0] = atoi(argv[2]);
    node_ports[1] = atoi(argv[3]);
    node_ports[2] = atoi(argv[4]);
    for(int i=0;i<number_of_nodes;i++){
        printf("Node port is %d\n", node_ports[i]);
    }
    //clean the file for each run.
    snprintf(fname, sizeof(fname), "%s","front-end.log");
    filename = fopen(fname, "w");
    fclose(filename);
    remove(fname);
    
    front_end_processor();
	return 0;
}

void front_end_processor(){
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
    int reuse = 1;
    setsockopt(socket_desc,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
        //print the error message
        perror("bind failed. Error");
    }
    printf("bind done\n");
    
    //Listen
    listen(socket_desc , 50);
    while((client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)))
    {     
        //Accept and incoming connection
        // printf("Waiting for communication with the other nodes.\n"); 
        printf("Waiting for clients..\n");
        // client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        if (client_sock < 0){
            perror("accept failed");
        }
        else{
            pthread_t send_server;
             new_sock = (int*)malloc(sizeof(int));
            *new_sock = client_sock;
            int n = pthread_create( &send_server , NULL ,  new_client_thread , (void*) new_sock);      
            if( n  < 0){
              perror("Could not create receiver thread");
            }
            //pthread_join(send_server, NULL);
        }
    }
    close(client_sock);
 }

 void * new_client_thread(void *new_sock){
    int client_sock = *(int*)new_sock;
    char message[2000];
    while(1){
        recv(client_sock, message,2000,0);
        printf("***********************************************\n");
        printf("Front end received message %s\n", message);
        //printf("Log file name %s\n",fname);
        FILE* filename;
        filename = fopen(fname, "a+");
        if (filename == NULL) { /* Something is wrong   */}
        fprintf(filename, "Message from client %s\n", message);
        fclose(filename);
        
        //Abort if any one server says to Abort.
        int operationstatus= send_to_all_servers(message,client_sock);
        if(operationstatus == -1){
            char serverreply[2000];
            strcpy(serverreply,"Aborted the transaction.");
            int sendval = send(client_sock,&serverreply,sizeof(serverreply),0); 
            if(sendval<0)
                printf("Error in send");
        }
    }
 }

int ready = 0, connected=0; char servermessage[2000];
int send_to_all_servers(char* client_message, int client_sock){
    //connect to the all servers and initiate 2 phase commit
    int socket_desc , c , *new_sock,sock;

    struct sockaddr_in data_server , client;
    char initmessage[2000];
    strcpy(initmessage,client_message);
    printf("-------------------------------------------------------\n");
    printf("Connecting to all servers to initiate 2-phase commit.\n");
    int sockids[number_of_nodes];
    for(int i=0;i<number_of_nodes;i++){
        //connect to all the servers and ask if ready
        sock = socket(AF_INET , SOCK_STREAM , 0);
        sockids[i] = sock;
        if(sock == -1){
        printf("Could not create socket");
        }
        data_server.sin_addr.s_addr = inet_addr("127.0.0.1");
        data_server.sin_family = AF_INET;
        data_server.sin_port = htons(node_ports[i]);
        
        //Connect to remote server
        if (connect(sock , (struct sockaddr *)&data_server , sizeof(data_server)) < 0){
            printf("Error in connecting.\n");
        }else{
            filename = fopen(fname, "a+");
            fprintf(filename, "Sending message to servers: %s\n", initmessage);
            fclose(filename);  
            printf("Sending message to servers %s\n", initmessage);
            int sendval = send(sock,&initmessage,sizeof(initmessage),0); 
            if(sendval<0)
                printf("Error in send");
            else{
                connected++;
                sleep(2);
                int recn = recv(sockids[i], servermessage,2000,0);
                
                if(recn > 0){
                    filename = fopen(fname, "a+");
                    fprintf(filename, "-------------------------------------------------\n");
                    fprintf(filename, "Server %d message %s\n",node_ports[i],servermessage);
                    fclose(filename);
                    printf("Server %d message is %s\n",node_ports[i],servermessage);
                    ready++;
                    char two_phase_response[2000];
                    if(strcmp(servermessage,"ABORT")==0){
                        printf("Aborting the transaction.\n");
                        abortop++;
                    }

                }else{
                    printf("Error receiving ready to commit message from server.\n");
                }
            }
        }     
    }
    //Close all the sockets before intiating the operation to perform the transaction- > where a new socket is created.
    for(int i=0;i<number_of_nodes;i++){
        close(sockids[i]);
    }
    if(abortop == 0 && ready > 1){
        int compute =0;
        for(int i=0;i<number_of_nodes;i++){
            //connect to all the servers and ask if ready
            sock = socket(AF_INET , SOCK_STREAM , 0);
            if(sock == -1){
            printf("Could not create socket");
            }
            
            data_server.sin_addr.s_addr = inet_addr("127.0.0.1");
            data_server.sin_family = AF_INET;
            data_server.sin_port = htons(node_ports[i]);
            if (connect(sock , (struct sockaddr *)&data_server , sizeof(data_server)) < 0){
                printf("Error in connecting.\n");
            }else{
                snprintf(initmessage, sizeof(initmessage), "%s %s", "COMMIT " ,client_message);
                // strcpy(initmessage, "COMMIT");
                printf("-----------------------------------------------------\n");
                printf("Sending commit to all servers.\n");
                sockids[i] = sock;
                int sendval = send(sockids[i],&initmessage,sizeof(initmessage),0); 
                if(sendval<0)
                    printf("Error in send");
                bzero(servermessage,2000);
                int recn = recv(sockids[i], servermessage,2000,0);
                if(recn > 0){
                    compute++;
                    printf("Server %d message is %s\n",node_ports[i],servermessage);
                } 

                if(compute > 1){
                    ready = 0; connected =0; compute =0;
                    printf("-----------------------------------------------------\n");
                    int sendval = send(client_sock,&servermessage,sizeof(servermessage),0); 
                    if(sendval<0){
                        printf("Error in sending\n");
                    }
                }else{
                    printf("Aborting the operation.\n");
                }
                close(sock);
            }
        }return 0;
    }else{
        abortop =0;
        //send the servers to abort the transaction.
        printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
        printf("Sending abort to all servers.\n");
        filename = fopen(fname, "a+");
        fprintf(filename, "Sending Abort messsage to all servers.\n");
        fclose(filename);
        snprintf(initmessage, sizeof(initmessage), "%s", "ABORT");
        for(int i=0;i<number_of_nodes;i++){
            sock = socket(AF_INET , SOCK_STREAM , 0);
            if(sock == -1){
            printf("Could not create socket");
            }
            data_server.sin_addr.s_addr = inet_addr("127.0.0.1");
            data_server.sin_family = AF_INET;
            data_server.sin_port = htons(node_ports[i]);
            if (connect(sock , (struct sockaddr *)&data_server , sizeof(data_server)) < 0){
                printf("Error in connecting.\n");
            }else{
                sockids[i] = sock;
                int sendval = send(sockids[i],&initmessage,sizeof(initmessage),0); 
                if(sendval<0)
                    printf("Error in send");
            }
        }
        return -1;
    }
     
}