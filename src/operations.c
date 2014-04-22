// Operations.c
// File with data storage operations

#include "operations.h"

int listenfd, ackfd, getackfd, num_processes, id, next_entry;
char * addresses;
int keys[100], values[100];
int * delays;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//////////////////////////////////////////////////////////////////////////////////
//Utilities

void init_operations(int lfd, int afd, int gafd, int num_p, char * new_addrs, int new_id, int * new_delays){
	listenfd = lfd;
	ackfd = afd;
	getackfd = gafd;
	num_processes = num_p;
	addresses = new_addrs;
	id = new_id;
	next_entry = 0;
	delays = new_delays;
}

void lock(){
	pthread_mutex_lock(&mutex);
}

void unlock(){
	pthread_mutex_unlock(&mutex);
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

int delay(int dest_id){
	int dest_delay = delays[dest_id];
	int random_delay = (dest_delay == 0 ? 0 : rand() % (2 * dest_delay));  //[0, 2*dest_delay-1]
    if(random_delay > 0){
    	usleep(random_delay*1000);
    }
	return 1;
}

//send message to one process
int unicast_send(int dest_id, char * payload, int channel){
	char buf[MAX_BUF_LEN];
	struct addrinfo * p;
	char * dest_ip = addresses + (dest_id * 16);
	int dest_port = 0;
	if(channel == 0) dest_port = PORT + dest_id;
	else if(channel == 1) dest_port = ACK_PORT + dest_id;
	else if(channel == 2) dest_port = GETACK_PORT + dest_id;
	else printf("FATAL: unicast_send unknown channel :%d\n", channel);
	int talkfd = set_up_talk(dest_ip, dest_port, &p);

	if(talkfd != -1){
		//delay(dest_id);
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
int unicast_receive(char * message, int * sender, int channel){
	char temp[MAX_BUF_LEN];
	int bytes = 0;
	delay(id);
	if(channel == 0) bytes = udp_listen(listenfd, temp);
	else if(channel == 1) bytes = udp_listen(ackfd, temp);
	else if(channel == 2) bytes = udp_listen(getackfd, temp);
	else printf("FATAL: unicast_receive unknown channel: %d\n", channel);
	if(sender != 0) *sender = *((int *)temp);
	memcpy(message, temp+HEADER_SIZE, MAX_BUF_LEN - HEADER_SIZE);
	return bytes - HEADER_SIZE;
}

//send one ack to one process
int send_ack(int dest_id){
	unicast_send(dest_id, "ack", 1);
	return 1;
}

//wait for all acks from replicas
int wait_for_acks(){
	char message[MAX_BUF_LEN];
	int count = 0;
	while(count < 2){
		int rec_bytes = unicast_receive(message, 0, 1);
		if(rec_bytes > 0){
			char prefix[30];
			sscanf(message, "%s ", prefix);
			if(strcmp(prefix, "ack") == 0){
				if(VERBOSE) printf("got ack\n");
				count++;
			}
			else{
				printf("FATAL: received message on ack port that isnt an ack\n");
				return -1;
			}
		}
	}
	return 1;
}

//send a get ack to one process
int send_get_ack(int dest_id, int getresult){
	char payload[30];
	sprintf(payload, "getack %d", getresult);
	unicast_send(dest_id, payload, 2);
	return 1;
}

//wait for all get acks from replicas
//results must be integer array of size 2
int wait_for_get_acks(int * results){
	char message[MAX_BUF_LEN];
	int count = 0;
	while(count < 2){
		int rec_bytes = unicast_receive(message, 0, 2);
		if(rec_bytes > 0){
			char prefix[30];
			sscanf(message, "%s ", prefix);
			if(strcmp(prefix, "getack") == 0){
				int getresult = 0;
				sscanf(message+7, "%d ", &getresult);
				if(VERBOSE) printf("got getack %d \n", getresult);
				results[count++] = getresult;
			}
			else{
				printf("FATAL: received message on get ack port that isnt a get ack\n");
				return -1;
			}
		}
	}
	return 1;
}

//wait for get response on ack channel
int return_result(){
	char message[MAX_BUF_LEN];
	int rec_bytes = unicast_receive(message, 0, 1);
	if(rec_bytes > 0){
		char prefix[10];
		sscanf(message, "%s ", prefix);
		if(strcmp(prefix, "result") == 0){
			int retval = 0;
			sscanf(message+7, "%d ", &retval);
			return retval;
		}
		else{
			printf("FATAL: did not received expected result\n");
		}
	}
	return -1;
}

//send get result to one process on ack channel
int send_result(int dest_id, int result){
	char payload[30];
	sprintf(payload, "result %d", result);
	unicast_send(dest_id, payload, 1);
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////
//Operations

int get(int key){
	lock();
	if(VERBOSE) printf("--> get key: %d\n", key);
	int pos = 0;
	while(pos < next_entry){
		if(keys[pos++] == key){
			unlock();
			return values[pos-1];
		}
	}
	unlock();
	return -1;	
}

int insert(int key, int value){
	lock();
	if(VERBOSE) printf("--> insert key: %d value: %d\n", key, value);
	keys[next_entry] = key;
	values[next_entry++] = value;
	unlock();
	return 1;
}

int update(int key, int value){
	lock();
	if(VERBOSE) printf("--> update key: %d value: %d\n", key, value);
	int pos = 0;
	while(pos < next_entry){
		if(keys[pos] == key){
			values[pos] = value;
		}
		pos++;
	}
	unlock();
	return 1;
}

int delete(int key){
	lock();
	if(VERBOSE) printf("--> delete key: %d\n", key);
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
	unlock();
	return 1;
}

int search(int key){
	lock();
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
	unlock();
	return 1;
}

int show_all(){
	lock();
	int pos = 0;
	while(pos < next_entry){
		printf("%2d | key: %2d value: %2d\n", pos, keys[pos], values[pos]);
		pos++;
	}
	unlock();
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////
// Operation Assisters

int contact_replicas(char * payload){
	int pos = 1;
	if(VERBOSE) printf("sending command to replicas:");
	for(pos = 1; pos < 3; pos++){
		if(VERBOSE) printf(" %d", (id+pos)%num_processes);
		if(VERBOSE) fflush(stdout);
		unicast_send((id+pos)%num_processes, payload, 0);
	}
	if(VERBOSE) printf("\n");
	return 1;
}

//Do operation or send message to owner
int op_jump_function(int key, int value, int level, char * payload, int op_code, int just_typed_in, int sender){
	int ret_val = -1;
	int am_owner = id == get_owner(key);
	int am_replica = is_replica(key, id);

	if(just_typed_in && !am_owner){
		unicast_send(get_owner(key), payload, 0); //contact owner
		return return_result(); //wait for owner to reply with result
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
		if(op_code == 0){   //get command
			int results[2];
			if(level > 1){  //wait for everyone
				if(VERBOSE) printf("waiting for all acks\n");
				wait_for_get_acks(results);
				if(VERBOSE) printf("get results from replicas: %d %d \n", results[0], results[1]);
				if(VERBOSE) printf("sending get result: %d\n", ret_val); //change return value to most recent write if inconsistent
			
				//check for inconsistency in results[] right here ----------------------------------------------------------
			}
			else{  //dont wait
				if(fork() == 0){  //child process to consume acks in background
					wait_for_get_acks(results);
					_exit(EXIT_SUCCESS);
					//check for inconsistency in results[] right here ----------------------------------------------------------
				}
			}
		}
		else{
			if(level > 1){
				if(VERBOSE) printf("waiting for all acks\n");
				wait_for_acks();
			}
			else{
				int pid = fork();
				if(pid == -1) printf("FATAL: unable to launch child process in op_jump_function\n");
				else if(pid == 0){ wait_for_acks(); _exit(EXIT_SUCCESS);} //child process to consume acks in background
			}
		}
		if(!just_typed_in) send_result(sender, ret_val); //only send result if this command wasnt typed into the owner
	}
	else if(am_replica){
		if(op_code == 0) send_get_ack(get_owner(key), ret_val);
		else send_ack(get_owner(key));
	}
	else{
		printf("FATAL: called op_jump_function on process that isnt owner or replica\n");
		return -1;
	}
	return ret_val;
}

