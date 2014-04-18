// Operations.c
// File with data storage operations

#include "operations.h"

int listenfd, num_processes, id;
char * addresses;

//////////////////////////////////////////////////////////////////////////////////
//Utilities

void init_operations(int lfd, int num_p, char * new_addrs, int new_id){
	listenfd = lfd;
	num_processes = num_p;
	addresses = new_addrs;
	id = new_id;
}

//return id of process in charge of this key
int get_owner(int key){
	return key % num_processes;
}

//set up message header
//update HEADER_SIZE constant in header file
int create_header(char * buf, int id){
	*((int *) buf) = id;
	return 1;
}

int delay(){
	return 1;
}

//send message to one process
int unicast_send(int dest_id, char * payload){
	char buf[MAX_BUF_LEN];
	struct addrinfo * p;
	char * dest_ip = addresses + (dest_id * 16);
	int dest_port = PORT + dest_id;
	int talkfd = set_up_talk(dest_ip, dest_port, &p);
	
	if(talkfd != -1){
		delay();
	
		create_header(buf, id);
		memcpy(buf+HEADER_SIZE, payload, MAX_BUF_LEN - HEADER_SIZE);
	
		udp_send(talkfd, buf, p);

		freeaddrinfo(p);
		close(talkfd);
		return 1;
	}
	else{
		printf("unicast send: cannot send to ip: %s port: %d\n", dest_ip, dest_port);
		return -1;
	}
}

//receive message from one process
int unicast_receive(char * message){
	char temp[MAX_BUF_LEN];
	int bytes = udp_listen(listenfd, temp);
	int sender = *((int *)temp);

	printf("%d> %s\n", sender, temp+HEADER_SIZE);

	memcpy(message, temp+HEADER_SIZE, MAX_BUF_LEN - HEADER_SIZE);

	return bytes - HEADER_SIZE;
}

//////////////////////////////////////////////////////////////////////////////////
//Operations

//parse received message and do operation
int listen_and_do(){
	char op[MAX_ARG_LEN];
	char command[MAX_BUF_LEN];
	int a1, a2, a3;
	int * arg1, * arg2, * arg3;
	arg1 = &a1;
	arg2 = &a2;
	arg3 = &a3;
	
	while(1){
		memset(command, 0, MAX_BUF_LEN);
		memset(op,   0, MAX_ARG_LEN);
		*arg1 = 0;
		*arg2 = 0;
		*arg3 = 0;

		int rec_bytes = unicast_receive(command);
		if(rec_bytes > 0){
			sscanf(command, "%s ", op);

			if(strcmp(op, "delete") == 0){
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
				printf("Received unknown command %s\n", op);
			}
		}
	}
}

int get(int key, int level){
	int owner = get_owner(key);

	if(id == owner){
		printf("get owner operation\n");
	}
	else{
		char payload[100];
		sprintf(payload, "get %d %d", key, level);
		unicast_send(get_owner(key), payload);
	}
	return 1;	
}

int insert(int key, int value, int level){
	int owner = get_owner(key);

	if(id == owner){
		printf("insert owner operation\n");
	}
	else{
		char payload[100];
		sprintf(payload, "insert %d %d %d", key, value, level);
		unicast_send(get_owner(key), payload);
	}
	return 1;
}

int update(int key, int value, int level){
	int owner = get_owner(key);

	if(id == owner){
		printf("update owner operation\n");
	}
	else{
		char payload[100];
		sprintf(payload, "update %d %d %d", key, value, level);
		unicast_send(get_owner(key), payload);
	}
	return 1;
}

int delete(int key){
	int owner = get_owner(key);

	if(id == owner){
		printf("delete owner operation\n");
	}
	else{
		char payload[100];
		sprintf(payload, "delete %d", key);
		unicast_send(get_owner(key), payload);
	}
	return 1;
}

int search(int key){
	int owner = get_owner(key);

	if(id == owner){
		printf("search owner operation\n");
	}
	else{
		char payload[100];
		sprintf(payload, "search %d", key);
		unicast_send(get_owner(key), payload);
	}
	return 1;
}

int show_all(){
	printf("show_all command\n");
	return 1;
}

