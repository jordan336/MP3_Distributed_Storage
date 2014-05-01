//Threads.c
//Code executed by command and message threads

#include "threads.h"

#define MAX_COM_LEN 100

typedef struct astruct{
	char * command;
	int sender;
	int timestamp;
	unsigned int * random;
} arg_struct;

//execute thread
//parse user input and do command or contact owner
void * do_commands(){
	char op[MAX_ARG_LEN]; 
	int a1, a2, a3, retval;
	int * arg1, * arg2, * arg3;
	arg1 = &a1;
	arg2 = &a2;
	arg3 = &a3;
	char command[MAX_COM_LEN];
	
	while(1){
		memset(command, 0, MAX_COM_LEN);
		memset(op,   0, MAX_ARG_LEN);
		*arg1 = 0;
		*arg2 = 0;
		*arg3 = 0;
		fgets(command, MAX_COM_LEN, stdin);
		sscanf(command, "%s ", op);

		if(strcmp(op, "quit") == 0){
			pthread_cancel(message_thread);
			return 0;
		}
		else if(strcmp(op, "delete") == 0){
			if(sscanf(command+7, "%d ", arg1) != 1) printf("Delete requires a key\n");
			else retval = op_jump_function(*arg1, 0, 0, 0, command, 3, 1, 0);
		}
		else if(strcmp(op, "get") == 0){
			if(sscanf(command+4, "%d %d ", arg1, arg2) != 2) printf("Get requires a key and a level\n");
			else{
				retval = op_jump_function(*arg1, 0, *arg2, 0, command, 0, 1, 0);
				if(retval != -1) printf("key: %d value: %d\n", *arg1, retval);
				else printf("key %d not found\n", *arg1);
			}
		}
		else if(strcmp(op, "insert") == 0){
			if(sscanf(command+7, "%d %d %d ", arg1, arg2, arg3) != 3) printf("Insert requires a key, value, and level\n");
			else retval = op_jump_function(*arg1, *arg2, *arg3, 0, command, 1, 1, 0);
		}
		else if(strcmp(op, "update") == 0){
			if(sscanf(command+7, "%d %d %d ", arg1, arg2, arg3) != 3) printf("Update requies a key, value, and level\n");
			else retval = op_jump_function(*arg1, *arg2, *arg3, 0, command, 2, 1, 0);
		}
		else if(strcmp(op, "show-all") == 0){
			retval = show_all();
		}
		else if(strcmp(op, "search") == 0){
			if(sscanf(command+7, "%d ", arg1) != 1) printf("Search requires a key\n");
			else retval = search(*arg1);
		}
		else{
			printf("Unknown command %s\n", op);
		}
	}	
	return 0;
}

//run by next thread for every command
void * execute_command(void * args){
	char op[MAX_ARG_LEN];
	int a1, a2, a3, retval;
	int * arg1, * arg2, * arg3;
	arg1 = &a1;
	arg2 = &a2;
	arg3 = &a3;
	*arg1 = 0;
	*arg2 = 0;
	*arg3 = 0;

	char * command = ((arg_struct *)args)->command;
	int sender = ((arg_struct *)args)->sender;
	int timestamp = ((arg_struct *)args)->timestamp;
	set_random_seed(((arg_struct *)args)->random);
	sscanf(command, "%s ", op);
	
	if(strcmp(op, "delete") == 0){
		if(sscanf(command+7, "%d ", arg1) != 1) printf("Delete requires a key\n");
		else retval = op_jump_function(*arg1, 0, 0, timestamp, command, 3, 0, sender);
	}
	else if(strcmp(op, "get") == 0){
		if(sscanf(command+4, "%d %d ", arg1, arg2) != 2) printf("Get requires a key and a level\n");
		else retval = op_jump_function(*arg1, 0, *arg2, timestamp, command, 0, 0, sender);
	}
	else if(strcmp(op, "insert") == 0){
		if(sscanf(command+7, "%d %d %d ", arg1, arg2, arg3) != 3) printf("Insert requires a key, value, and level\n");
		else retval = op_jump_function(*arg1, *arg2, *arg3, timestamp, command, 1, 0, sender);
	}
	else if(strcmp(op, "update") == 0){
		if(sscanf(command+7, "%d %d %d ", arg1, arg2, arg3) != 3) printf("Update requies a key, value, and level\n");
		else retval = op_jump_function(*arg1, *arg2, *arg3, timestamp, command, 2, 0, sender);
	}
	else{
		printf("Received unknown command %s\n", command);
	}
	free(command);
	return 0;
}

//receive thread
//receive messages sent to owner or replicas and do operation
void * do_messages(){
	while(1){
		int sender, timestamp;
		unsigned int random_num;
		char * command = (char *)malloc(MAX_BUF_LEN * sizeof(char));
		int rec_bytes = unicast_receive(command, &sender, &timestamp, 0);
		if(rec_bytes > 0){
			random_num = rand();
			arg_struct args;
			args.command = command;
			args.sender = sender;
			args.timestamp = timestamp;
			args.random = &random_num;
			pthread_t thread;
			pthread_create(&thread, NULL, &execute_command, (void *)&args);	
		}
	}

	return 0;
}

