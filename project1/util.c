#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "util.h"
int Client_arg = 5;
unsigned short Server_port_max = 18200;
unsigned short Server_port_min = 18000;
int Msg_size_min = 10;
int Msg_size_max = 65535;
int Block_size_min = 1;
int Block_size_max = 10000;
int BACKLOG = 5;


/***************CLIENT UTIL******************/
int validateUserInput(int argc, unsigned short server_port, unsigned short size_msg, int count){

	if ((argc != Client_arg) ||(server_port < Server_port_min || server_port > Server_port_max) ||(size_msg < Msg_size_min || size_msg > Msg_size_max) || (count < Block_size_min || count > Block_size_max)) {
		return 0;
	}
	return 1;
}

void initClient(char** pbuffer, char** psendbuffer, unsigned short size_msg, int* psock, struct sockaddr_in* psin, unsigned short  server_port, unsigned int server_addr)
{
	*pbuffer = (char *) malloc(size_msg);
	if (!(*pbuffer))
	{
		perror("failed to allocated buffer");
		abort();
	}

	*psendbuffer = (char *) malloc(size_msg);
	if (!(*psendbuffer))
	{
		perror("failed to allocated sendbuffer");
		abort();
	}
	/* create a socket */
	if ((*psock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		perror ("opening TCP socket");
		abort ();
	}

	/* fill in the server's address */
	memset (psin, 0, sizeof (*psin));
	psin->sin_family = AF_INET;
	psin->sin_addr.s_addr = server_addr;
	psin->sin_port = htons(server_port);

	/* connect to the server */
	if (connect(*psock, (struct sockaddr *)psin, sizeof (*psin)) < 0)
	{
		perror("connect to server failed");
		abort();
	}

}

void setTimeStamp(char* sendbuffer, struct timeval* tv){
	gettimeofday(tv,NULL);
	*(int*)(sendbuffer + 2) = (int)htons(tv->tv_sec);
	*(int*)(sendbuffer + 2 + 4) = (int)htons(tv->tv_usec);
}

/**********SERVER UTIL*******************/
void initServer(int* psock,unsigned short server_port, int BACKLOG, struct sockaddr_in* psin, int optval){
	/* create a server socket to listen for TCP connection requests */
	if ((*psock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		perror ("opening TCP socket");
		abort ();
	}

	/* set option so we can reuse the port number quickly after a restart */
	if (setsockopt (*psock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (optval)) <0)
	{
		perror ("setting TCP socket option");
		abort ();
	}

	/* fill in the address of the server socket */
	memset (psin, 0, sizeof (*psin));
	psin->sin_family = AF_INET;
	psin->sin_addr.s_addr = INADDR_ANY;
	psin->sin_port = htons (server_port);

	/* bind server socket to the address */
	if (bind(*psock, (struct sockaddr *)psin, sizeof (*psin)) < 0)
	{
		perror("binding socket to address");
		abort();
	}

	/* put the server socket in listen mode */
	if (listen (*psock, BACKLOG) < 0)
	{
		perror ("listen on socket failed");
		abort();
	}
}

int serverSending(struct node* current){
	int cnt;
	/*use non blocking mechanism*/
	cnt = send(current->socket, current->buffer + current->added_pos, current->len_buffer - current->added_pos,MSG_DONTWAIT);
	if (cnt > 0){
		current->added_pos = cnt;
		/*if the total msg has been sent, reset the node*/
		if (current->added_pos == current->len_buffer){
			current->added_pos = 0;
			current->pending_data = 0;
			current->len_buffer = 0;
		}
	}
	return cnt;
}

int serverRecieving(struct node* current){
	int cnt;

    /*use non blocking mechanism*/
    cnt = recv(current->socket, current->buffer+current->added_pos, Msg_size_max - current->added_pos, MSG_DONTWAIT);
	if (cnt > 0){
		if(current->len_buffer == 0){
            current->len_buffer = ntohs(*(unsigned short*)current->buffer);/*get the length of message to be received*/
		}
		current->added_pos += cnt;
		/*if the total msg has been received, enable the write flag of this node*/
		if(current->added_pos == current->len_buffer){
			current->pending_data = 1;
			current->added_pos = 0;
		}
	}
	return cnt;
}

/* remove the data structure associated with a connected socket
   used when tearing down the connection */
void dump(struct node* head, int socket) {
	struct node *current, *temp;

	current = head;

	while (current->next) {
		if (current->next->socket == socket) {
			/* remove */
			temp = current->next;
			current->next = temp->next;
			free(temp->buffer);
			free(temp); /* don't forget to free memory */
			return;
		} else {
			current = current->next;
		}
	}
}

/* create the data structure associated with a connected socket */
void add(struct node* head, int socket, struct sockaddr_in addr) {
	struct node* new_node;

	new_node = (struct node *)malloc(sizeof(struct node));
	new_node->socket = socket;
	new_node->client_addr = addr;
	new_node->pending_data = 0;
	new_node->buffer = (char*)malloc(Msg_size_max);
	new_node->added_pos = 0;
	new_node->len_buffer = 0;
	new_node->next = head->next;
	head->next = new_node;
}
