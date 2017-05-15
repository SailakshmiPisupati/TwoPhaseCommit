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
int number_of_nodes=2;

struct account_details{
    int account_no;
    int account_status;
    pthread_mutex_t lock_account;
};

void front_end_processor();
void* receivemessages(void* arg);
void send_to_all_servers(char* message);
int get_data(int connect_port, char * message);
int get_random_server();
int perform_two_phase_commit();

int main(int argc, char *argv[])
{
	front_end_port = atoi(argv[1]);
    node_ports[0] = atoi(argv[2]);
    node_ports[1] = atoi(argv[3]);
    // node_ports[2] = atoi(argv[4]);

    for(int i=0;i<number_of_nodes;i++){
        printf("Node port is %d\n", node_ports[i]);
    }
    //read the input file
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
    char message[2000];
    FILE *filename;

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
    while(1){     
        //Accept and incoming connection
        // printf("Waiting for communication with the other nodes.\n"); 
        client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        if (client_sock < 0){
            perror("accept failed");
        }
        else{
            int recn = recv(client_sock, message,2000,0);
            if(recn>0){
                printf("***********************************************\n");
                printf("Front end received message %s\n", message);
                char content[100];
                snprintf(content, sizeof(content), "%s %s %s", "Message received from client: ",message,"\n");
                //printf("Log file name %s\n",fname);

                char fname[200];
                snprintf(fname, sizeof(fname), "%s " ,"front-end.log");
                filename = fopen(fname, "a+");
                if (filename == NULL) { /* Something is wrong   */}
                fprintf(filename, content);
                fclose(filename);
                send_to_all_servers(message);  
            }
        }
    close(client_sock);
    }
 }

int ready = 0, connected=0; 
void send_to_all_servers(char* client_message){
    //connect to the other two servers and initiate 2 phase commit
    int socket_desc , c , *new_sock,sock;
    int client_sock;
    struct sockaddr_in data_server , client;
    char initmessage[2000], servermessage[2000];
    strcpy(initmessage,client_message);
    printf("-------------------------------------------------------\n");
    printf("Connecting to other servers to initiate 2-phase commit.\n");
    int sockids[number_of_nodes];
    for(int i=0;i<number_of_nodes;i++){
        //connect to all the servers and ask if ready
        sock = socket(AF_INET , SOCK_STREAM , 0);
        sockids[i] = sock;
        if(sock == -1){
        printf("Could not create socket");
        }
        printf("Node port to connect %d\n", node_ports[i]);
        data_server.sin_addr.s_addr = inet_addr("127.0.0.1");
        data_server.sin_family = AF_INET;
        data_server.sin_port = htons(node_ports[i]);

        //Connect to remote server
        if (connect(sock , (struct sockaddr *)&data_server , sizeof(data_server)) < 0){
        perror("connect failed. Error");
        }else{  
            
            
            printf("Sending message to servers %s\n", initmessage);
            int sendval = send(sock,&initmessage,sizeof(initmessage),0); 
            if(sendval<0)
                printf("Error in send");
            else{
                connected++;
                printf("Checking if servers are ready\n");
            }
        }
    }
    if(connected!=0){
        time_t start;
        time_t stop;
        time(&start);
        
        int diff =0;
        do{
            time(&stop);
            diff = difftime(stop, start);
            printf("diff time is %d\n", diff);
            for(int i=0;i<number_of_nodes;i++){
            int recn = recv(sockids[i], servermessage,2000,0);
            if(recn > 0){
                printf("Server %d message is %s\n",node_ports[i],servermessage);
                ready++;
            } 
        }
        }while(diff<=30 || connected!=ready);
        
    }
    int compute =0;
    if(connected ==  ready){
        printf("Connect count : %d and Ready count: %d\n", connected,ready);
        for(int i=0;i<number_of_nodes;i++){
        // snprintf(initmessage, sizeof(initmessage), "%s %s", "COMMIT " ,initmessage);
        strcpy(initmessage, "COMMIT");
        printf("Sending commit to all servers.\n");
        int sendval = send(sockids[i],&initmessage,sizeof(initmessage),0); 
        if(sendval<0)
            printf("Error in send");
        bzero(servermessage,2000);
        int recn = recv(sockids[i], servermessage,2000,0);
        if(recn > 0){
            compute++;
            printf("Server %d message is %s\n",node_ports[i],servermessage);
        } 

        if(compute == ready){
            printf("Sending file to client..\n");
        }
        }
    }else{
        printf("Aborting transaction.\n");
    }

     
}