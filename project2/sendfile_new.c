#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>     /* for pause */
#include <signal.h>     /* for signal */
#include "packet.h"
#include "crc16.h"


#define WINDOW_LEN 8
#define SEQ_LEN 16
#define ALPHA 0.5  
#define BUFLEN 1024
typedef unsigned short u_short;

struct alarm_time{
	int is_set;
	int tv_sec;
	int tv_usec;
};
struct alarm_time alarm_list[SEQ_LEN];
#include "alarm.h"
int timeout;
char** str_split(char* a_str, const char a_delim);

int la_seq;		/* the least acknowledge sequence number */
int ns_seq;		/* the sequence number to be sent next */
int total_packets; /* the total number of data packets to be sent */
char pck_window[WINDOW_LEN][BUFLEN+18];//the packets in the window

int main(int argc, char** argv)
{
	struct sockaddr_in sin; 
	int sock, i;
	unsigned int slen=sizeof(sin);
	char buf[BUFLEN];	/* message buffer */
	char *file_buf, *packet;
	int recvlen;		/* # bytes in acknowledgement message */
	int rtt;		/* round trip time in millisecond */
	int count_last_packet = 0; /* count the number of retransmission the last packet. if >100, consider the server ends*/
	struct timeval tv;
	//struct hostent *host = gethostbyname(argv[1]);

	char** token = str_split(argv[2],':');
	printf("%s\n",token[0]);
	printf("%s\n",token[1]);
	//struct hostent *host = gethostbyname(token[0]);
	char* file_name = argv[4];	

	/* 0: not used
	 * 1: being transmitted, waiting for ack
	 * 2: ack received
	 */

    for(i=0;i<SEQ_LEN;i++){
        alarm_list[i].is_set = 0;
    }

	/* server port number */
	unsigned short server_port = atoi (token[1]);

	/* create a socket */

	if ((sock=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		printf("socket created\n");

	/* bind it to the server addresses and use the given port number */
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	//sin.sin_addr.s_addr = server_addr;
	sin.sin_addr.s_addr = inet_addr(token[0]);
	sin.sin_port = htons(server_port);

	FILE *fp = fopen(file_name, "r");
	/* get the size of the file */
	fseek(fp,0l,SEEK_END);
	int file_size = ftell(fp);
	total_packets = file_size/BUFLEN+1;
	rewind(fp);
	//printf("The file size is %d\n",file_size);


	/* initial packet*/
	timeout = 7500;
	packet = (char*)malloc((strlen(file_name)+10)*sizeof(char)); 
	gettimeofday(&tv,NULL);
	int start_sec = tv.tv_sec;
	int start_usec = tv.tv_usec;
	/* send a ping-pong message to get the rtt */
	*(u_short *) (packet+2) = htons(WINDOW_LEN-1); /* -1 means it is a ping packet*/
	*(u_short *) (packet+4) = htons(0);
	*(u_short *) (packet+6) = htons(2);//regular type
	memcpy(packet+8, file_name, strlen(file_name)+1);
	u_short check_sum = calc_crc16(packet+2, strlen(file_name)+7);
	*(u_short *) (packet) = htons(check_sum);
	int elapsed_time;
	int first_time = 1;
	while(1){
		gettimeofday(&tv,NULL);
		elapsed_time = (tv.tv_sec-start_sec)*1000000+(tv.tv_usec - start_usec);
		if(first_time || elapsed_time > timeout){
			first_time = 0;
			start_sec = tv.tv_sec;
			start_usec = tv.tv_usec;
			if (sendto(sock, packet, strlen(file_name)+9, 0, (struct sockaddr *)&sin, slen) ==-1) {
				perror("buffer sending failure\n");
			}
		}
		recvlen = recvfrom(sock, buf, BUFLEN, MSG_DONTWAIT, (struct sockaddr *)&sin, &slen);
		if (recvlen > 0){
			check_sum = calc_crc16(buf+2, recvlen-2);
			if(ntohs(*(u_short *)buf) != check_sum){
				printf("[recv corrupt packet]\n");
				continue;
			}
			else{
				if(htons(*(u_short *)(buf+6))==1 && htons(*(u_short *)(buf+4))==WINDOW_LEN-1){
					break;
				}
			}
		}
	}
	
	/* read file to a buffer with BUFLEN length and send it to the sever continuously*/
	u_short ack_num;
	packet = (char*)malloc(BUFLEN*sizeof(char));
	file_buf = (char*)malloc(BUFLEN*sizeof(char));
	if (fp != NULL) {

		i = 0;
		la_seq = ns_seq = 0;
        int last_window = -1;
                    
		file_buf = (char*)malloc(BUFLEN*sizeof(char));
		while(1) {
            check_timeout();
			/*  Keep waiting for acknowledgements from the server */
			/*  Make the recvfrom non blocking to receive latter acknowledgements */
			recvlen = recvfrom(sock, packet, BUFLEN, MSG_DONTWAIT, (struct sockaddr *)&sin, &slen);
			if (recvlen >= 0) {
				gettimeofday(&tv, NULL)
				/* check the correctness of the packet using md5 */
				check_sum = calc_crc16(packet+2, recvlen-2);
				if (ntohs(*(u_short *)packet, check_sum)) {
					printf("[recv corrupt packet]\n");
					continue;
				}
				ack_num = ntohs(*(u_short *)(packet+4));
				/* The ack_num should be greater or equal to zero */
				/* Ignore the packet that has already arrived */
				if(ack_num < 0 || ack_num >= SEQ_LEN) { 
					continue;
				}
                if(between(la_seq, (la_seq+WINDOW_LEN-1)%SEQ_LEN, ack_num)){
                    if(ack_num == last_window){
                        break;
                    }
                    for(i = la_seq; i < la_seq+WINDOW_LEN; i++){
                        if(between(la_seq, ack_num, i % SEQ_LEN)) alarm_list[i%SEQ_LEN].is_set = 0;
                    }
                    la_seq = (ack_num+1)%SEQ_LEN;
                }
                
				/* update the rtt based on the new rtt */
				int newRtt = 1000000*(tv.tv_sec - alarm_list[ack_num].tv_sec) + (tv.tv_usec-alarm_list[ack_num].tv_usec);
				timeout = newRtt*1.1;
            }
            //send new window
            for(i=la_seq; i < la_seq + WINDOW_LEN; i++) {
                int k = i % SEQ_LEN;
                if(!alarm_list[k].is_set){
                    int new_len = fread(pck_window[i-la_seq], sizeof(char), BUFLEN, fp);
                    /* create the ftp packet */
                    *(u_short *) (pck_window[i-la_seq]+2) = htons(k); /* 1 means it is a ftp packet */
                    *(u_short *) (pck_window[i-la_seq]+4) = htons(0);
                    
                    if (new_len < BUFLEN) {
                        last_window = k;
						*(u_short *) (pck_window[i-la_seq]+6) = htons(2);

                    }
                    else {
						*(u_short *) (pck_window[i-la_seq]+6) = htons(2);
					}

					checksum = calc_crc16(pck_window[i-la_seq]+2, new_len+6);
                    *(u_short *) (pct_window[i-la_seq]) = htons(checksum);

                    printf("[sending data %d] start (%d)\n", k, new_len);
                    set_alarm(k);

                    if (sendto(sock, pck_window[i-la_seq], new_len+8, 0, (struct sockaddr *)&sin, slen) ==-1) {
                        perror("buffer sending failure\n");
                        exit(1);
                    }
                }
            } // end of transmit new file
		}
		fclose(fp);
	}
	close(sock);
	exit(0);
}

/* id is the sequence number of a packet and duration is the length of the timeout */
void set_alarm (int id) {
	struct timeval tv;
	gettimeofday(&tv,NULL);
	alarm_list[id].is_set = 1;
	alarm_list[id].tv_sec = tv.tv_sec;
	alarm_list[id].tv_usec = tv.tv_usec;
}

/* iterate through all the packets being transmitted and check for timeout expires */
void check_timeout(int timeout) {
    int i;
    int k;
    struct timeval tv;
    for(i=la_seq;i<la_seq+WINDOW_LEN;i++){
        k = i%SEQ_LEN;
        if(alarm_list[k].is_set){
            gettimeofday(&tv,NULL);
            int elapsed_time = (tv.tv_sec-alarm_list[i].tv_sec)*1000000+(tv.tv_usec - alarm_list[i].tv_usec);
            if(elapsed_time > timeout){
				alarm_list[k].is_set = 0;
            }
        }
    }
}

char** str_split(char* a_str, const char a_delim)
{
	char** result    = 0;
	size_t count     = 0;
	char* tmp        = a_str;
	char* last_comma = 0;
	char delim[2];
	delim[0] = a_delim;
	delim[1] = 0;

	/* Count how many elements will be extracted. */
	while (*tmp)
	{
		if (a_delim == *tmp)
		{
			count++;
			last_comma = tmp;
		}
		tmp++;
	}

	/* Add space for trailing token. */
	count += last_comma < (a_str + strlen(a_str) - 1);

	/* Add space for terminating null string so caller
	   knows where the list of returned strings ends. */
	count++;

	result = malloc(sizeof(char*) * count);

	if (result)
	{
		size_t idx  = 0;
		char* token = strtok(a_str, delim);

		while (token)
		{
			assert(idx < count);
			*(result + idx++) = strdup(token);
			token = strtok(0, delim);
		}
		assert(idx == count - 1);
		*(result + idx) = 0;
	}

	return result;
}
//./sendfile -r 128.42.208.5:18000 -f usa.txt
