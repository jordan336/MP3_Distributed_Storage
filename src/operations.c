// Operations.c
// File with data storage operations

#include "operations.h"

struct get_ret_s{
	int ret_val;
	int timestamp;
};

struct owner_action_t{
	int opcode;
	char payload[MAX_BUF_LEN];
	int timestamp;
	int get_val;
	int get_time;
};

int listenfd, ackfd, getackfd, num_processes, id, next_entry, cur_time;
char * addresses;
int keys[100], values[100], timestamps[100];
int * delays;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
unsigned int * random_seed;

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
	cur_time = 0;
	random_seed = (unsigned int *)malloc(sizeof (unsigned int));
	*random_seed = rand();
}

void lock(){
	pthread_mutex_lock(&mutex);
}

void unlock(){
	pthread_mutex_unlock(&mutex);
}

void set_random_seed(unsigned int * new_random_seed){
	random_seed = new_random_seed;
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
int create_header(char * buf, int id, int timestamp){
	*((int *)buf) = id;
	*(((int *)buf)+1) = timestamp;
	return 1;
}

int delay(int dest_id){
	int dest_delay = delays[dest_id];
	int random_delay = (dest_delay == 0 ? 0 : rand_r(random_seed) % (2 * dest_delay));  //[0, 2*dest_delay-1]
    if(random_delay > 0){
		if(VERBOSE) printf("delaying %d\n", random_delay);
    	usleep(random_delay*1000);
    }
	return 1;
}

//returns 1 if inconsistency found in values
int is_inconsistent(int ret_val, int * results){
	return (ret_val != results[0] || ret_val != results[1]);
}

//returns newest value based on timestamp between ret_val and results
int get_newest(int ret_val, int timestamp, int * results, int * rec_timestamps){
	if(timestamp > rec_timestamps[0] && timestamp > rec_timestamps[1]){
		return ret_val;
	}
	else if(rec_timestamps[0] > timestamp && rec_timestamps[0] > rec_timestamps[1]){
		return results[0];
	}
	else{
		return results[1];
	}
}


//////////////////////////////////////////////////////////////////////////////////
// Message passing

//send message to one process
int unicast_send(int dest_id, char * payload, int channel, int timestamp){
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
		delay(dest_id);
		create_header(buf, id, timestamp);
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
int unicast_receive(char * message, int * sender, int * timestamp, int channel){
	char temp[MAX_BUF_LEN];
	int bytes = 0;
	//delay(id);
	if(channel == 0) bytes = udp_listen(listenfd, temp);
	else if(channel == 1) bytes = udp_listen(ackfd, temp);
	else if(channel == 2) bytes = udp_listen(getackfd, temp);
	else printf("FATAL: unicast_receive unknown channel: %d\n", channel);
	if(sender != 0) *sender = *((int *)temp);
	if(timestamp != 0) *timestamp = *(((int *)temp)+1);
	memcpy(message, temp+HEADER_SIZE, MAX_BUF_LEN - HEADER_SIZE);
	return bytes - HEADER_SIZE;
}

//send one ack to one process
int send_ack(int dest_id){
	unicast_send(dest_id, "ack", 1, -1);
	return 1;
}

//wait for all acks from replicas
int wait_for_acks(){
	char message[MAX_BUF_LEN];
	int count = 0;
	while(count < 2){
		int rec_bytes = unicast_receive(message, 0, 0, 1);
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

//replica send a get ack to one process
int send_get_ack(int dest_id, int getresult, int timestamp){
	char payload[30];
	sprintf(payload, "getack %d %d", getresult, timestamp);
	unicast_send(dest_id, payload, 2, -1);
	return 1;
}

//owner wait for all get acks from replicas
//results must be integer array of size 2
int wait_for_get_acks(int * results, int * rec_timestamps){
	char message[MAX_BUF_LEN];
	int count = 0;
	while(count < 2){
		int rec_bytes = unicast_receive(message, 0, 0, 2);
		if(rec_bytes > 0){
			char prefix[30];
			sscanf(message, "%s ", prefix);
			if(strcmp(prefix, "getack") == 0){
				int getresult = 0;
				int rec_timestamp = 0;
				sscanf(message+7, "%d %d ", &getresult, &rec_timestamp);
				if(VERBOSE) printf("got getack %d timestamp: %d\n", getresult, rec_timestamp);
				rec_timestamps[count] = rec_timestamp;
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

//coordinator wait for get response from owner on ack channel
int return_result(){
	char message[MAX_BUF_LEN];
	int rec_bytes = unicast_receive(message, 0, 0, 1);
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

//send get result to coordinator from owner on ack channel
int send_result(int dest_id, int result){
	char payload[30];
	sprintf(payload, "result %d", result);
	unicast_send(dest_id, payload, 1, -1);
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////
//Operations

struct get_ret_s get(int key){
	struct get_ret_s to_ret;
	lock();
	printf("--> get key: %d\n", key);
	int pos = 0;
	while(pos < next_entry){
		if(keys[pos++] == key){
			unlock();
			to_ret.ret_val = values[pos-1];
			to_ret.timestamp = timestamps[pos-1];
			return to_ret;
		}
	}
	unlock();
	to_ret.ret_val = -1;
	to_ret.timestamp = -1;
	return to_ret;	
}

int insert(int key, int value, int timestamp){
	lock();
	printf("--> insert key: %d value: %d\n", key, value);
	keys[next_entry] = key;
	values[next_entry] = value;
	timestamps[next_entry++] = timestamp;
	unlock();
	return 1;
}

int update(int key, int value, int timestamp){
	lock();
	printf("--> update key: %d value: %d timestamp: %d\n", key, value, timestamp);
	int pos = 0;
	while(pos < next_entry){
		if(keys[pos] == key){
			values[pos] = value;
			timestamps[pos] = timestamp;
		}
		pos++;
	}
	unlock();
	return 1;
}

int delete(int key){
	lock();
	printf("--> delete key: %d\n", key);
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
		timestamps[x] = timestamps[pos];
		values[x++] = values[pos++];	
	}
	if(did_delete) next_entry--;
	unlock();
	return 1;
}

int search(int key){
	int pos = 0;
	int owner, rep1, rep2;
	owner = get_owner(key);
	rep1 = 0;
	rep2 = 0;
	char payload[50];
	sprintf(payload, "get %d %d", key, 10);
	unicast_send(owner, payload, 0, -1);
	int ret_val = return_result();
	if(ret_val == -1){
		printf("Key: %d not found\n", key);
	}
	else{
		lock();
		while(pos < num_processes){
			if(is_replica(key, pos)){
				if(rep1 == 0) rep1 = pos;
				else rep2 = pos;
			}
			pos++;
		}
		printf("owner: %d | replicas: %d %d\n", owner, rep1, rep2)	;
		unlock();	
	}

	return ret_val;
}

int show_all(){
	lock();
	int pos = 0;
	while(pos < next_entry){
		//printf("%2d | key: %2d value: %2d\n", pos, keys[pos], values[pos]);
		printf("%2d | key: %2d value: %2d timestamp: %2d\n", pos, keys[pos], values[pos], timestamps[pos]);
		pos++;
	}
	unlock();
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////
// Operation Assisters

int contact_replicas(char * payload, int timestamp){
	int pos = 1;
	if(VERBOSE) printf("sending command to replicas:");
	for(pos = 1; pos < 3; pos++){
		if(VERBOSE) printf(" %d", (id+pos)%num_processes);
		if(VERBOSE) fflush(stdout);
		unicast_send((id+pos)%num_processes, payload, 0, timestamp);
	}
	if(VERBOSE) printf("\n");
	return 1;
}

//contact replicas and check for inconsistency
int owner_action(int opcode, char * payload, int timestamp, int get_val, int get_time){
	int ret_val = get_val;
	contact_replicas(payload, timestamp);
	if(opcode == 0){   //get command
		int results[2];
		int rec_timestamps[2];
		wait_for_get_acks(results, rec_timestamps);
		if(VERBOSE) printf("get results from replicas: %d %d \n", results[0], results[1]);
		if(VERBOSE) printf("get timestamps from replicas: %d %d \n", rec_timestamps[0], rec_timestamps[1]);
		int inconsistent = is_inconsistent(get_val, results);	
		if(inconsistent != 0){
			ret_val = get_newest(get_val, get_time, results, rec_timestamps);
			printf("repairing data\n");
			int key = 0;
			char repair_payload[50];
			sscanf(payload, "%*s %d ", &key);
			sprintf(repair_payload, "update %d %d %d", key, ret_val, -1);
			update(key, ret_val, timestamp);
			contact_replicas(repair_payload, timestamp);
			wait_for_acks();
		}	
	}
	else{
		wait_for_acks();
	}
	return ret_val;
}

void * struct_owner_action(void * args){
	struct owner_action_t * arg_t = (struct owner_action_t *)args;
	owner_action(arg_t->opcode, arg_t->payload, arg_t->timestamp, arg_t->get_val, arg_t->get_time);
	free(arg_t);
	return 0;
}

//Do operation or send message to owner
int op_jump_function(int key, int value, int level, int ts, char * payload, int op_code, int just_typed_in, int sender){
	struct get_ret_s grstruct;
	int ret_val = -1;
	int get_time = -1;
	int timestamp = ts;
	int am_owner = id == get_owner(key);
	int am_replica = is_replica(key, id);

	if(just_typed_in && !am_owner){
		unicast_send(get_owner(key), payload, 0, -1); //contact owner
		return return_result(); //wait for owner to reply with result
	}

	if(am_owner){
		timestamp = ++cur_time;
	}

	switch(op_code){
		case 0:
			grstruct = get(key);
			ret_val = grstruct.ret_val;
			get_time = grstruct.timestamp;
			break;
		case 1:
			ret_val = insert(key, value, timestamp);
			break;
		case 2:
			ret_val = update(key, value, timestamp);
			break;
		case 3:
			ret_val = delete(key);
			break;
		default:
			printf("Unknown operation code %d\n", op_code);
			break;
	}

	if(am_owner){
		if(level > 1){
			ret_val = owner_action(op_code, payload, timestamp, ret_val, get_time);
			if(!just_typed_in) send_result(sender, ret_val); //only send result to coordinator if this command wasnt typed into the owner
		}
		else{
			if(!just_typed_in) send_result(sender, ret_val); //respond immediately 
			struct owner_action_t * args;
			args = (struct owner_action_t *)malloc(sizeof(struct owner_action_t));
			memcpy(args->payload, payload, MAX_BUF_LEN);
			args->opcode = op_code;
			args->timestamp = timestamp;
			args->get_val = ret_val;
			args->get_time = get_time;
			pthread_t thread;
			pthread_create(&thread, NULL, &struct_owner_action, (void *)args);
		}
	}
	else if(am_replica){
		if(op_code == 0) send_get_ack(get_owner(key), ret_val, get_time);
		else send_ack(get_owner(key));
	}
	else{
		printf("FATAL: called op_jump_function on process that isnt owner or replica\n");
		return -1;
	}
	return ret_val;
}

