#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>
#include "util.h"
/* simple client, takes two parameters, the server domain name,
   and the server port number */

int main(int argc, char** argv) {

	/* our client socket */
	int sock;

	/* address structure for identifying the server */
	struct sockaddr_in sin;

	/* convert server domain name to IP address */
	struct hostent *host = gethostbyname(argv[1]);
	unsigned int server_addr = *(unsigned int *) host->h_addr_list[0];

	/* server port number */
	unsigned short server_port = atoi (argv[2]);
	

	/*message size*/
	unsigned short size_msg = atoi(argv[3]);
	/* num of msgs*/
	int count = atoi(argv[4]);

	if (!validateUserInput(argc, server_port, size_msg, count)){
		perror("Invalid input arguments!");
		abort();
	}

	char *buffer = NULL;
	char *sendbuffer = NULL;

	/* allocate a memory buffer in the heap */
	/* putting a buffer on the stack like:

	   char buffer[500];

	   leaves the potential for
	   buffer overflow vulnerability */
	initClient(&buffer, &sendbuffer, size_msg, &sock, &sin, server_port, server_addr); 

	if (!sendbuffer)
	{
		perror("failed to allocated sendbuffer");
		abort();
	}

	/* everything looks good, since we are expecting a
	   message from the server in this example, let's try receiving a
	   message from the socket. this call will block until some data
	   has been received */

	long double total_latency, avr_latency;
	total_latency = 0.;
	avr_latency = 0.;
	struct timeval start, end;
	*(short*) sendbuffer  = (short)htons(size_msg);/*set the length of msg to be sent*/
	//int dt = 0;
	int num_tests = 0;
	long double cur_latency = 0.;
    while(num_tests < count){
		setTimeStamp(sendbuffer, &start);
		int cnt = 0;
		while(cnt < size_msg){
			cnt += send(sock, sendbuffer+cnt, size_msg-cnt, 0);
			//printf("%d\n",cnt);
		}
		cnt = 0;
		while(cnt < size_msg){
            cnt = recv(sock, buffer+cnt, size_msg-cnt,0);
			//printf("%d, %d\n",cnt, dt);
		}
		gettimeofday(&end,NULL);
		
        cur_latency = (end.tv_sec - start.tv_sec)*pow(10.,6) + end.tv_usec-start.tv_usec; 
        total_latency += cur_latency;
		num_tests += 1;
    }
	
	avr_latency = total_latency/(count*pow(10.,3));

	printf("The average latency is: %Lf ms\n", avr_latency);    

	/* free the resources, generally important! */
	close(sock);
	free(buffer);
	free(sendbuffer);
	return 0;
}
