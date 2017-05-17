#include <stdio.h>
#include <string.h>    
#include <stdlib.h>    
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>   
#include <pthread.h>
#include <stdbool.h>

void *sendmessages(void *socket_desc);
void *receivemessages(void *socket_desc);
void create_sender_receiver_thread(short server_port, short connect_to_new);
void *sendmessage_toclient(void *socket_desc);
void compute_transaction(char* transaction,int client_sock);
void write_to_file(int account_number, int transaction_amount);
int read_from_file(int account_no);

short server_port, connect_to;

int total_messages=100, last_account_no =0; 
int message_buffer[100];
static int new_account_no =100, account_count =0; char result[2000];
pthread_mutex_t lock_account = PTHREAD_MUTEX_INITIALIZER;
char fname[200];

struct account_details{
    int account_number;
    int account_amount;
};
struct account_details bank_accounts[2];

int main(int argc , char *argv[]){
    
    int *new_sock;
    int i=0;
    for(i=0;i<total_messages;i++){
      message_buffer[i] = -1;                       //set the value of the messages at first to one.
    }
    server_port = atoi(argv[1]);
    connect_to = atoi(argv[2]);
    printf("In main thread.\n");
    FILE *fp;
    
    snprintf(fname, sizeof(fname), "%s%d%s ","server",server_port ,".log");
    fp = fopen(fname, "w");
    fclose(fp);

    int ret = remove(fname);
    char fname[200];
                    
    if(ret == 0){
        printf("File deleted successfully");
    }
    create_sender_receiver_thread(server_port, connect_to);               //sender is the server
    return 0;
}


void create_sender_receiver_thread(short server_port,short connect_to){
    printf("In create receiver thread\n");
    pthread_t sniffer_thread;
     // a thread to receive the incoming messages from front-end application.
    int n = pthread_create( &sniffer_thread , NULL ,  receivemessages , NULL);      
    if( n  < 0){
      perror("Could not create receiver thread");
    }
    pthread_join(sniffer_thread, NULL);
}

void *receivemessages(void *args){

    //int server_port = *(int*)server_port_new;
    int socket_desc , c , *new_sock;
    int client_sock;
    struct sockaddr_in server , client;
    char client_message[2000], transaction[2000];
    FILE *filename;
    //Create socket
    
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket\n");
    }
    printf("Server up and running....\n");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(server_port);
    printf("Node port is %d\n", server_port);
    //Bind
    int reuse = 1;
    setsockopt(socket_desc,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
        //print the error message
        perror("bind failed. Error");
    }
    printf("bind done\n");
    
    //Listen
    listen(socket_desc , 3);
    while(1){
        client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        if (client_sock < 0){
            perror("accept failed");
        }else{
        bzero(transaction,2000);
        int recn = recv(client_sock, transaction,2000,0);
        if(recn>0){
            char *msg = strtok(transaction," ");
            int strcompare = strcmp(msg,"COMMIT");
            if(strcompare == 0){
                char *new_transaction = strtok(NULL,"\n");
                filename = fopen(fname,"a+");
                fprintf(filename, "Server will now compute the values\n");
                fclose(filename);
                printf("Server will now compute\n");
                compute_transaction(new_transaction,client_sock);
                printf("Computating result %s\n", result);
                int sendval = send(client_sock,&result,sizeof(result),0); 
                if(sendval<0){
                    printf("Error in sending\n");
                }
                close(client_sock);   
            }else{
                printf("***********************************************\n");    
                if(strcmp(transaction,"ABORT")==0){
                    printf("Front end :%s\n", transaction);
                    filename = fopen(fname,"a+");
                    fprintf(filename, "Message from front-end is %s\n", transaction);
                    fclose(filename);
                }else{
                    char *new_transaction = strtok(NULL,"\n");
                    printf("Message from front-end %s\n", new_transaction);
                    printf("Message received %s\n", new_transaction);
                    filename = fopen(fname,"a+");
                    fprintf(filename, "*********************************************\n");
                    fprintf(filename, "Message from front-end is %s\n", new_transaction);
                    fclose(filename);
                    srand(time(NULL));
                    int random = (rand() %10)+1;
                    if(random%10==0){
                        strcpy(transaction,"ABORT");
                    }else{
                        strcpy(transaction,"READY TO COMMIT");    
                    }
                    filename = fopen(fname,"a+");
                    fprintf(filename, "Server will %s\n", transaction);
                    fclose(filename);
                    
                    printf("Sending message %s to front end application. \n",transaction);
                    int n = send(client_sock,&transaction,sizeof(transaction),0); 
                    if(n<0)
                        printf("Error in send");

                    int recn = recv(client_sock, transaction,2000,0);
                    if(recn>0){
                        printf("Front end said to : %s\n",transaction);
                        filename = fopen(fname,"a+");
                        fprintf(filename, "Server will %s\n", transaction);
                        fclose(filename);
                    }
                }  
            } 
        }  
        }   
    }  
    close(socket_desc); 
}


void compute_transaction(char* transaction,int client_sock){
    printf("compute transaction %s\n", transaction);
    char *type, *transaction_amount, *account_no;
    
    type = strtok(transaction," ");
    
    
    printf("Type of request is %s\n", type);
    if((strcmp(type,"CREATE"))==0 ||(strcmp(type,"Create"))==0||(strcmp(type,"create"))==0){
        pthread_mutex_lock(&lock_account);
        transaction_amount = strtok(NULL," ");
        new_account_no++;
        bank_accounts[new_account_no].account_number = new_account_no;
        bank_accounts[new_account_no].account_amount = atoi(transaction_amount);
        snprintf(result, sizeof(result), "%s %d","OK",new_account_no);
        account_count++;
        pthread_mutex_unlock(&lock_account);
    }else if((strcmp(type,"UPDATE"))==0||(strcmp(type,"Update"))==0||(strcmp(type,"update"))==0){
        transaction_amount = strtok(NULL," ");
        account_no = strtok(NULL, "\n");
        int account_count = atoi(account_no);
        printf("Account count is %d\n", account_count);
        if(bank_accounts[account_count].account_number == 0){
            printf("Account not present.\n");
            snprintf(result, sizeof(result), "%s %s%s","Err. Account",account_no," not found");
        }else{
            pthread_mutex_lock(&lock_account);
            int amount = bank_accounts[account_count].account_amount;
            amount = amount + (atoi(transaction_amount));
            bank_accounts[account_count].account_amount = amount;
            snprintf(result, sizeof(result), "%s %d","OK", bank_accounts[account_count].account_amount);
            pthread_mutex_unlock(&lock_account);
        }
    }else if((strcmp(type,"QUERY"))==0||(strcmp(type,"Query"))==0||(strcmp(type,"query"))==0){
        account_no = strtok(NULL," ");
        int account_count = atoi(account_no);
        if(bank_accounts[account_count].account_number == 0){
            printf("Account not present\n");
            snprintf(result, sizeof(result), "%s %s%s","Err. Account",account_no," not found");
        }else{
            int amount = bank_accounts[account_count].account_amount;
            snprintf(result, sizeof(result), "%s %d","OK", bank_accounts[account_count].account_amount);
        }    
    }

    int sendval = send(client_sock,&result,sizeof(result),0); 
    if(sendval<0){
        printf("ERrror in sending\n");
    }
}

void write_to_file(int account_number, int transaction_amount){
    printf("Writing to file...\n");
    FILE * filename;
    char content[100];
    snprintf(content, sizeof(content), "%d %d%s",account_number,transaction_amount,"\n");
    //printf("Log file name %s\n",fname);

    char fname[200];
    snprintf(fname, sizeof(fname), "%s%d%s ","server",server_port ,".txt");
    filename = fopen(fname, "a+");
    if (filename == NULL) { /* Something is wrong   */}
    fprintf(filename, content);
    fclose(filename);
}

int read_from_file(int account_no){
    printf("Reading file to get the account balance for %d\n", account_no);
    int transaction_amount;
    FILE *get_records; 
    char fname[200];char message[200];
    snprintf(fname, sizeof(fname), "%s%d%s ","server",server_port ,".txt");
    get_records =fopen(fname,"r");
    if(!get_records)
    printf("Unable to open file\n");

    while(fgets(message,2000,get_records)){
      int acc_no = atoi(strtok(message, " "));
      if(acc_no == account_no){
        transaction_amount = atoi(strtok(NULL, " "));
      }else{
        transaction_amount = -1;
      }
    }
    fclose(get_records);
    printf("Transaction amount is %d\n",transaction_amount);
    return transaction_amount;
}
