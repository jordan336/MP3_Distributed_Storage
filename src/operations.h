//Header file for operations.c

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "networking.h"

#define PORT 15457
#define ACK_PORT 25457
#define GETACK_PORT 35457
#define MAX_BUF_LEN 1000
#define MAX_ARG_LEN 50
#define HEADER_SIZE 1 * sizeof(int)
#define VERBOSE 1

void init_operations(int lfd, int afd, int gafd, int num_p, char * new_addrs, int new_id, int * new_delays);
int unicast_receive(char * message, int * sender, int channel);
int show_all();
int search(int key);
int op_jump_function(int key, int value, int level, char * payload, int op_code, int just_typed_in, int sender);

