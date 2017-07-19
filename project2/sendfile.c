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
#include "crc16.h"
#include "util.h"
#include "fix_queue.h"


struct alarm_time{
	int is_set;
	int reload;
	int size;
	int tv_sec;
	int tv_usec;
};
struct alarm_time alarm_list[SEQ_LEN];
char** str_split(char* a_str, const char a_delim);
void set_alarm(int);
void check_timeout(int);

int la_seq;		/* the least acknowledge sequence number */
int ns_seq;		/* the sequence number to be sent next */
uint32_t packet_id = 0;
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
		alarm_list[i].reload = 1;
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
	rewind(fp);
	//printf("The file size is %d\n",file_size);


	/* initial packet*/
	int timeout = 5000;


	packet = (char*)malloc((strlen(file_name)+10)*sizeof(char)); 
	gettimeofday(&tv,NULL);
	int start_sec = tv.tv_sec;
	int start_usec = tv.tv_usec;

	// send subdir and filename and set rtt
	*(u_short *) (packet+2) = htons(SEQ_LEN-1);
	*(u_short *) (packet+4) = htons(0);//seq ID
	*(u_short *) (packet+6) = htons(2);//regular type
	*(uint32_t *) (packet+8) = htons(0); //0 to label filename
    memcpy(packet+8+4, file_name, strlen(file_name)+1);
	u_short check_sum = calc_crc16(packet+2, strlen(file_name)+7+4);
	*(u_short *) (packet) = htons(check_sum);
	int elapsed_time;
	int first_time = 1;
	uint32_t last_ack_time = 0;
	uint32_t tmp_ack_sec;
	uint32_t tmp_ack_usec;
	uint32_t tmp_time;

	while(1){
		gettimeofday(&tv,NULL);
		elapsed_time = (tv.tv_sec-start_sec)*1000000+(tv.tv_usec - start_usec);
		if(first_time || elapsed_time > timeout){
			first_time = 0;
			start_sec = tv.tv_sec;
			start_usec = tv.tv_usec;
			if (sendto(sock, packet, strlen(file_name)+9+4, 0, (struct sockaddr *)&sin, slen) ==-1) {
				perror("buffer sending failure\n");
			}
		}
		recvlen = recvfrom(sock, buf, BUFLEN, MSG_DONTWAIT, (struct sockaddr *)&sin, &slen);
		if (recvlen > 0){
			check_sum = calc_crc16(buf+2, recvlen-2);
			if(ntohs(*(u_short *)buf) != check_sum){
				printf("[recv corrupt packet]\n");
				//continue;
			}
			else{
				tmp_ack_sec = ntohl(*(uint32_t *)(buf+8));
				tmp_ack_usec = ntohl(*(uint32_t *)(buf+12));
				tmp_time = tmp_ack_usec + 1000000*tmp_ack_sec; 

				if(tmp_time > last_ack_time && htons(*(u_short *)(buf+6))==1 && htons(*(u_short *)(buf+4))==SEQ_LEN-1){
					last_ack_time = tmp_time;
					break;
				}
			}
		}
	}
	
	u_short ack_num;
	//packet = (char*)malloc(BUFLEN*sizeof(char));
	Node* root = NULL;
	if (fp != NULL) {

		i = 0;
		la_seq = ns_seq = 0;
        int last_window = -1;
                    
		while(1) {
            check_timeout(timeout);
			recvlen = recvfrom(sock, buf, BUFLEN, MSG_DONTWAIT, (struct sockaddr *)&sin, &slen);
			if (recvlen > 0) {
				gettimeofday(&tv, NULL);
				check_sum = calc_crc16(buf+2, recvlen-2);
				if (ntohs(*(u_short *)buf) != check_sum) {
					printf("[recv corrupt packet]\n");
					//continue;
				}
				else {
				tmp_ack_sec = ntohl(*(uint32_t *)(buf+8));
				tmp_ack_usec = ntohl(*(uint32_t *)(buf+12));
				tmp_time = tmp_ack_usec + 1000000*tmp_ack_sec; 

				if (tmp_time > last_ack_time){
					last_ack_time = tmp_time;

					ack_num = ntohs(*(u_short *)(buf+4));
                    if(ack_num < 0 || ack_num >= SEQ_LEN) { 
						continue;
					}

					if(between(la_seq, (la_seq+WINDOW_LEN-1)%SEQ_LEN, ack_num)){
						if(ack_num == last_window){
							break;
						}
						for(i = la_seq; i < la_seq+WINDOW_LEN; i++){
							if(between(la_seq, ack_num, mod(i, SEQ_LEN))) {//clear the flags for acked pack
								//printf("[Recv ack %d]\n", ack_num);
								alarm_list[i%SEQ_LEN].is_set = 0;
								//alarm_list[i%SEQ_LEN].reload = 1;
								Dequeue(&root);
							}
						}
						la_seq = (ack_num+1)%SEQ_LEN;
					}

					/* update the rtt based on the new rtt */
					int newRtt = 1000000*(tv.tv_sec - alarm_list[ack_num].tv_sec) + (tv.tv_usec-alarm_list[ack_num].tv_usec);
					timeout = newRtt*1.1 < 5000 ? newRtt*1.1: 5000;
				}
				}
            }
            //send new window
            for(i=la_seq; i < la_seq + WINDOW_LEN; i++) {
                int k = i % SEQ_LEN;
				int win_id = i-la_seq;
                if(last_window != -1 && i > last_window){
                    break;
                }
                if((!alarm_list[k].is_set || alarm_list[k].is_set == 2)){
					Node* node_id = GetK(root, win_id);
					if(alarm_list[k].is_set != 2){
						if(node_id == NULL){
							file_buf = (char*)malloc((BUFLEN+12+4)*sizeof(char));
							node_id = (Node*)malloc(sizeof(Node));
							node_id->data = file_buf;
							node_id->next = NULL;
							Enqueue(&root, node_id);
						}
                        packet_id ++;
						alarm_list[k].size = fread(node_id->data+8+4, sizeof(char), BUFLEN, fp);
						/* create the ftp packet */
						*(u_short *) (node_id->data+2) = htons(k); //seq id
						*(u_short *) (node_id->data+4) = htons(0);

						if (alarm_list[k].size < BUFLEN) {
							last_window = k;
							*(u_short *) (node_id->data+6) = htons(4);//type 4: the last packet

						}
						else {
							*(u_short *) (node_id->data+6) = htons(2);
						}
                        
                        *(uint32_t *) (node_id->data+8) = htonl(packet_id);
						
                        check_sum = calc_crc16(node_id->data+2, alarm_list[k].size+6+4);
						*(u_short *) (node_id->data) = htons(check_sum);
					}

                    printf("[sending data %d] start (%d)\n", k, alarm_list[k].size);
                    set_alarm(k);

                    if (sendto(sock, node_id->data, alarm_list[k].size+8+4, 0, (struct sockaddr *)&sin, slen) ==-1) {
                        perror("buffer sending failure\n");
                        exit(1);
                    }
                }
            } // end of transmit new file
		}
		fclose(fp);
        free(packet);
        while(root) Dequeue(&root);
	}
    printf("[Completed!]\n");
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
            int elapsed_time = (tv.tv_sec-alarm_list[k].tv_sec)*1000000+(tv.tv_usec - alarm_list[k].tv_usec);
            if(elapsed_time > timeout){
				alarm_list[k].is_set = 2;//time out tag
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
