//Threads.c
//Code executed by command and message threads

#include "threads.h"

#define MAX_COM_LEN 100

void * do_commands(){
	char op[MAX_ARG_LEN]; 
	int a1, a2, a3;
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
			else delete(*arg1);
		}
		else if(strcmp(op, "get") == 0){
			if(sscanf(command+4, "%d %d ", arg1, arg2) != 2) printf("Get requires a key and a level\n");
			else get(*arg1, *arg2);
		}
		else if(strcmp(op, "insert") == 0){
			if(sscanf(command+7, "%d %d %d ", arg1, arg2, arg3) != 3) printf("Insert requires a key, value, and level\n");
			else insert(*arg1, *arg2, *arg3);
		}
		else if(strcmp(op, "update") == 0){
			if(sscanf(command+7, "%d %d %d ", arg1, arg2, arg3) != 3) printf("Update requies a key, value, and level\n");
			else update(*arg1, *arg2, *arg3);
		}
		else if(strcmp(op, "show-all") == 0){
			show_all();
		}
		else if(strcmp(op, "search") == 0){
			if(sscanf(command+7, "%d ", arg1) != 1) printf("Search requires a key\n");
			else search(*arg1);
		}
		else{
			printf("Unknown command %s\n", op);
		}
	}	
	return 0;
}

void * do_messages(){
	listen_and_do();
	return 0;
}

