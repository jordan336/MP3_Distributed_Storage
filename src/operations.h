//Header file for operations.c

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "networking.h"

#define PORT 15457
#define ACK_PORT 25457
#define MAX_BUF_LEN 1000
#define MAX_ARG_LEN 50
#define HEADER_SIZE 1 * sizeof(int)

void init_operations(int lfd, int afd, int num_p, char * new_addrs, int new_id);
int unicast_receive(char * message, int fd);
int show_all();
int search(int key);
int op_jump_function(int key, int value, int level, char * payload, int op_code, int just_typed_in);

