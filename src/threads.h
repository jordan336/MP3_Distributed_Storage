//Header file for threads.c

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

pthread_t command_thread, message_thread;

void * do_commands();
void * do_messages();

