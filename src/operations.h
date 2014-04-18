//Header file for operations.c

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "networking.h"

#define PORT 15457
#define MAX_BUF_LEN 1000
#define MAX_ARG_LEN 50
#define HEADER_SIZE 1 * sizeof(int)

void init_operations(int lfd, int num_p, char * new_addrs, int new_id);
int listen_and_do();
int get(int key, int level);
int insert(int key, int value, int level);
int update(int key, int value, int level);
int delete(int key);
int search(int key);
int show_all();

