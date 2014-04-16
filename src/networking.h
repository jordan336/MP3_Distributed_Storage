//Networking header file

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#define localhost "127.0.0.1"
#define max_buf_len 1000

int udp_listen(int sockfd, char * buf);
int udp_send(int sockfd, char * message, struct addrinfo * p);
int set_up_talk(char *address, int port_num, struct addrinfo **p);
int set_up_listen(int port_num, int nonblocking);

