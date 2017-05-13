#include <stdio.h>
#include <string.h>    
#include <stdlib.h>    
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>   
#include <pthread.h>
#include <stdbool.h>
#include <time.h>



int main(int argc, char const *argv[])
{
	int socket_desc , c , *new_sock;
	int client_sock;
	struct sockaddr_in server , client;

	return 0;
}


int create_account(int transation_amount){
    //write to file or memory the new transaction
    account_no = last_created_account + 1;
    accounts[account_no] = transation_amount;
    pthread_t commit;
    int n = pthread_create( &commit , NULL ,  init_two_phase_commit , NULL);      
    if( n  < 0){
      perror("Could not create receiver thread");
    }
    pthread_join(commit, NULL);
    return account_no;
}
