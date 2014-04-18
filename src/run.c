/********************************
* Run.c       					*
*********************************
* Mark Kennedy: kenned31		*
* Jordan Ebel : ebel1			*
*********************************
* Description					*
*								*
* Code run by every process	    *
********************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "networking.h"
#include "file_io.h"
#include "threads.h"
#include "operations.h"

#define VERBOSE 1

void print_status(char * addresses, int * delays, int num_processes){
    printf("Num processes: %d\n", num_processes);
    printf("IP Addresses | Delays\n");
    int i = 0;
    for(i=0; i<num_processes; i++){
        printf(" %s   |  %d \n", addresses+(i*16), delays[i]);
    }
    printf("---------------------------------\n");
}

void pthread_setup(){
	if(pthread_create(&command_thread, NULL, &do_commands, NULL)){
		printf("Command thread create error\n");
	}
	if(pthread_create(&message_thread, NULL, &do_messages, NULL)){
		printf("Message thread create error\n");
	}
	pthread_join(command_thread, NULL);
	pthread_join(message_thread, NULL);
}

int teardown(char * addresses, int * delays, int listenfd){
	free(addresses);
	free(delays);
	close(listenfd);
    return 1;
}

int main (int argc, const char* argv[]){
 
    int id, num_processes, listenfd;
    char * addresses;
	int * delays;    

    if(argc != 3){
        printf("store usage: config_file id\n");
        return -1;
    }
    else{
        addresses     = parse_config(argv[1], &num_processes, &delays);
		id            = atoi(argv[2]);
		if(num_processes < 4){
			printf("Must have at least 4 servers in the cluster\n");
			return -1;
		}
    }

    if(addresses == NULL) return -1;  //failed to read config file

    if(VERBOSE) print_status(addresses, delays, num_processes);
  
	listenfd = set_up_listen(PORT+id, 0);  //socket for receiving messages
 
	init_operations(listenfd, num_processes, addresses, id);
	pthread_setup();
	teardown(addresses, delays, listenfd);
 
	return 0;
}


