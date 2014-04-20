// Operations.c
// File with data storage operations

#include "operations.h"

int listenfd, ackfd, num_processes, id, next_entry;
char * addresses;
int keys[100], values[100];

//////////////////////////////////////////////////////////////////////////////////
//Utilities

void init_operations(int lfd, int afd, int num_p, char * new_addrs, int new_id){
	listenfd = lfd;
	ackfd = afd;
	num_processes = num_p;
	addresses = new_addrs;
	id = new_id;
	next_entry = 0;
}

//return id of process in charge of this key
int get_owner(int key){
	return key % num_processes;
}

//return 1 if id is a replica for this key, but not the owner
int is_replica(int key, int id){
	int owner = get_owner(key);
	if(id != get_owner(key) && ((owner+1)%num_processes == id || (owner+2)%num_processes == id)) return 1;
	return 0;
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
int unicast_send(int dest_id, char * payload, int fd){
	char buf[MAX_BUF_LEN];
	struct addrinfo * p;
	char * dest_ip = addresses + (dest_id * 16);
	int dest_port = 0;
	if(fd) dest_port = PORT + dest_id;
	else dest_port = ACK_PORT + dest_id;
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
int unicast_receive(char * message, int fd){
	char temp[MAX_BUF_LEN];
	int bytes = 0;
	if(fd) bytes = udp_listen(listenfd, temp);
	else bytes = udp_listen(ackfd, temp);
	//int sender = *((int *)temp);
	//printf("%d> %s\n", sender, temp+HEADER_SIZE);
	memcpy(message, temp+HEADER_SIZE, MAX_BUF_LEN - HEADER_SIZE);
	return bytes - HEADER_SIZE;
}

//send one ack to one process
int send_ack(int dest_id){
	unicast_send(dest_id, "ack", 0);
	return 1;
}

//wait for all acks from replicas
int wait_for_acks(){
	char message[MAX_BUF_LEN];
	int count = 0;
	while(count < 2){
		int rec_bytes = unicast_receive(message, 0);
		if(rec_bytes > 0){
			if(strcmp(message, "ack") == 0){

				printf("got ack\n");

				count++;
			}
			else{
				printf("FATAL: received message on ack port that isnt an ack\n");
				return -1;
			}
		}
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////
//Operations

int get(int key){
	printf("get key: %d\n", key);
	int pos = 0;
	while(pos < next_entry){
		if(keys[pos++] == key){
			return values[pos];
		}
	}
	return -1;	
}

int insert(int key, int value){
	printf("insert key: %d value: %d\n", key, value);
	keys[next_entry] = key;
	values[next_entry++] = value;
	return 1;
}

int update(int key, int value){
	printf("update key: %d value: %d\n", key, value);
	int pos = 0;
	while(pos < next_entry){
		if(keys[pos] == key){
			values[pos] = value;
		}
		pos++;
	}
	return 1;
}

int delete(int key){
	printf("delete key: %d\n", key);
	int pos = 0;
	int x = 0;
	int did_delete = 0;
	while(pos < next_entry){
		if(keys[pos] == key){
			pos++;
			did_delete = 1;
			continue;
		}
		keys[x] = keys[pos];
		values[x++] = values[pos++];	
	}
	if(did_delete) next_entry--;
	return 1;
}

int search(int key){
	printf("owner: %d | ", get_owner(key));
	printf("replicas: ");
	int pos = 0;
	while(pos < num_processes){
		if(is_replica(key, pos)){
			printf("%d ", pos);
		}
		pos++;
	}
	printf("\n");
	return 1;
}

int show_all(){
	int pos = 0;
	while(pos < next_entry){
		printf("%2d | key: %2d value: %2d\n", pos, keys[pos], values[pos]);
		pos++;
	}
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////
// Operation Assisters

int contact_replicas(char * payload){
	int pos = 1;
	for(pos = 1; pos < 3; pos++){
		printf("sending message to replica: %d\n", (id+pos)%num_processes);
		unicast_send((id+pos)%num_processes, payload, 1);
	}
	return 1;
}

//Do operation or send message to owner
int op_jump_function(int key, int value, int level, char * payload, int op_code, int just_typed_in){
	int ret_val = -1;
	int am_owner = id == get_owner(key);
	int am_replica = is_replica(key, id);

	if(just_typed_in && !am_owner){
		unicast_send(get_owner(key), payload, 1);	//contact owner
		return 1;
	}

	switch(op_code){
		case 0:
			ret_val = get(key);
			break;
		case 1:
			ret_val = insert(key, value);
			break;
		case 2:
			ret_val = update(key, value);
			break;
		case 3:
			ret_val = delete(key);
			break;
		default:
			printf("Unknown operation code %d\n", op_code);
			break;
	}
	if(am_owner){
		contact_replicas(payload);
		wait_for_acks();  //use level parameter to do consistency level here
	}
	else if(am_replica){
		printf("replica sending ack\n");
		send_ack(get_owner(key));
	}
	else{
		printf("FATAL: called op_jump_function on process that isnt owner or replica\n");
		return -1;
	}
	return 1;
}

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

		int rec_bytes = unicast_receive(command, 1);

		if(rec_bytes > 0){
			sscanf(command, "%s ", op);

			if(strcmp(op, "delete") == 0){
				if(sscanf(command+7, "%d ", arg1) != 1) printf("Delete requires a key\n");
				else op_jump_function(*arg1, 0, 0, command, 3, 0);
			}
			else if(strcmp(op, "get") == 0){
				if(sscanf(command+4, "%d %d ", arg1, arg2) != 2) printf("Get requires a key and a level\n");
				else op_jump_function(*arg1, 0, *arg2, command, 0, 0);
			}
			else if(strcmp(op, "insert") == 0){
				if(sscanf(command+7, "%d %d %d ", arg1, arg2, arg3) != 3) printf("Insert requires a key, value, and level\n");
				else op_jump_function(*arg1, *arg2, *arg3, command, 1, 0);
			}
			else if(strcmp(op, "update") == 0){
				if(sscanf(command+7, "%d %d %d ", arg1, arg2, arg3) != 3) printf("Update requies a key, value, and level\n");
				else op_jump_function(*arg1, *arg2, *arg3, command, 2, 0);
			}
			else{
				printf("Received unknown command %s\n", command);
			}
		}
	}
}

