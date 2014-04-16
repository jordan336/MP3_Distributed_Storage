//Threads.c
//Code executed by command and message threads

#include "threads.h"

#define MAX_COM_LEN 100
#define MAX_ARG_LEN 50

void * do_commands(){
	char op[MAX_ARG_LEN], arg1[MAX_ARG_LEN], arg2[MAX_ARG_LEN], arg3[MAX_ARG_LEN];
	char command[MAX_COM_LEN];
	
	while(1){
		memset(command, 0, MAX_COM_LEN);
		memset(op,   0, MAX_ARG_LEN);
		memset(arg1, 0, MAX_ARG_LEN);
		memset(arg2, 0, MAX_ARG_LEN);
		memset(arg3, 0, MAX_ARG_LEN);
		fgets(command, MAX_COM_LEN, stdin);
		sscanf(command, "%s ", op);

		if(strcmp(op, "quit") == 0){
			pthread_cancel(message_thread);
			return 0;
		}
		else if(strcmp(op, "delete") == 0){
			if(sscanf(command+7, "%s ", arg1) != 1) printf("Delete requires a key\n");
			else printf("Delete key: %s\n", arg1);
		}
		else if(strcmp(op, "get") == 0){
			if(sscanf(command+4, "%s %s ", arg1, arg2) != 2) printf("Get requires a key and a level\n");
			else printf("Get key: %s level: %s\n", arg1, arg2);
		}
		else if(strcmp(op, "insert") == 0){
			if(sscanf(command+7, "%s %s %s ", arg1, arg2, arg3) != 3) printf("Insert requires a key, value, and level\n");
			else printf("Insert key: %s value: %s level: %s\n", arg1, arg2, arg3);
		}
		else if(strcmp(op, "update") == 0){
			if(sscanf(command+7, "%s %s %s ", arg1, arg2, arg3) != 3) printf("Update requies a key, value, and level\n");
			else printf("Update key: %s value: %s level: %s\n", arg1, arg2, arg3);
		}
		else if(strcmp(op, "show-all") == 0){
			printf("Show-all command\n");
		}
		else if(strcmp(op, "search") == 0){
			if(sscanf(command+7, "%s ", arg1) != 1) printf("Search requires a key\n");
			else printf("Search for key: %s\n", arg1);
		}
		else{
			printf("Unknown command %s\n", op);
		}
	}	
	return 0;
}

void * do_messages(){
	return 0;
}

