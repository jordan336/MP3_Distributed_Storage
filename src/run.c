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

int teardown(){
    return 1;
}

int main (int argc, const char* argv[]){
 
    int id, num_processes;
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

		/* int i = 3;
		while(i < argc){
			int time = atoi(argv[i]);
			if(time < 0){
				printf("Delay times must be positive.\n");
				return -1;
			}
			else{
				printf("i: %d time: %d\n", i, time);
				delays[i++] = time;
			}
		} */
    }

    if(addresses == NULL) return -1;  //failed to read config file

    if(VERBOSE) print_status(addresses, delays, num_processes);
    
	return 0;
}


