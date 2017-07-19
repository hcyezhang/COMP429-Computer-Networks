#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include "crc16.h"

#define BUFLEN 1024
#define WINDOW_LEN 8
#define SEQ_LEN 16
#define PCKSIZE 1024
typedef unsigned short u_short;

int main(int argc, char **argv)
{
	int i;
	struct sockaddr_in sin;	/* our address */
	struct sockaddr_in remaddr;	/* remote address */
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	int recvlen;			/* # bytes received */
	int fd;				/* our socket */
	char buf[PCKSIZE];	/* receive buffer */
	char* packet;			/* sending the ack packet */
	unsigned short server_port = atoi(argv[2]);
	char file_name[1024];
	FILE *fp;			/* pointer to the destination file */

	int remaining_file_size = -1;	/* The number of bits remaining to be received */
	int remaining_packets;		/* The number of packets that have not been received */	
	int last_seq;			/* The index of the last sequence number */
	int ready_flag = 0;		/* Identify whether all the data has been received */
	int first_pingpong = 0;


	char flag[] = "-p";
	if(strcmp(argv[1],flag) != 0){
		perror("Invalid flag\n");
		return 0;
	}

	/* 
	 * 0 if packet is not received
	 * 1 if packet is received
	 */


	/* create a UDP socket */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return 0;
	}

	/* bind the socket to any valid IP address and a specific port */

	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(server_port);

	if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("bind failed");
		exit(1);
	}

	char windows[WINDOW_LEN][BUFLEN];
	int window_lengths[WINDOW_LEN];
	int la_seq = 0;
	int received[SEQ_LEN];
	int count = 0;
	int ind;
	for(i=0;i<SEQ_LEN;i++){
		received[i]=0;
		window_lengths[i]=0;
	}

	u_short check_sum = 0;
	packet = (char*)malloc(8*sizeof(char));
	while(1){
		recvlen = recvfrom(fd, buf, PCKSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
		if (recvlen > 0){
			check_sum = calc_crc16(buf+2, recvlen-2);
			if(ntohs(*(u_short *)(buf)) != check_sum){
				printf("[recv corrupt packet]\n");
				continue;
			}
			else{
				//copy buf to file_name
				memcpy(file_name, buf+8, recvlen-8);
				*(u_short *) (packet+2) = htons(0);//seq num
				*(u_short *) (packet+4) = htons(WINDOW_LEN-1);//ack num
				*(u_short *) (packet+6) = htons(1);//ack type
				check_sum = calc_crc16(packet+2, 6);
				*(u_short *) (packet) = htons(check_sum);
				printf("[sending ack %d]\n", WINDOW_LEN-1); 
				if (sendto(fd, packet, 8, 0, (struct sockaddr *)&remaddr, addrlen) < 0)
					perror("sendto");
				break;
			}
		}
	}

	/* now loop, receiving data and write the payload into a given file */
	printf("waiting on port %d\n", server_port);
	strcat(file_name, "_recv");
	printf("write to %s\n", file_name);
	fp = fopen(file_name, "w");
	if(fp == NULL){
		perror("open file error");
		exit(1);
	}
	int ready_exit = 0;
	int last_window = -1;

	while(1) {
		recvlen = recvfrom(fd, buf, PCKSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
		if (recvlen > 0) {
			//buf[recvlen] = 0;
			/* check the correctness of the packet using md5 */
			check_sum = calc_crc16(buf+2, recvlen-2);
			if (ntohs(*(u_short *)buf) != check_sum) {
				printf("[recv corrupt packet]\n");
				continue;
			}
			u_short type = ntohs(*(u_short *)(buf+6));

			if(type == 2 || type == 4){
				u_short seq_num = ntohs(*(u_short*)(buf+2));
				if(type == 4) {
					ready_exit = 1;
					last_window = seq_num;
				}

				/* Should be Accepted!*/
				if(between(la_seq, (la_seq+WINDOW_LEN-1)%SEQ_LEN, seq_num)&& !received[seq_num]) {
					int file_buf_size = recvlen-8; /* the received buffer size minus the headers' size */					
					if (seq_num > la_seq){
						received[seq_num] = 1;
						count = 0;
						printf("[Recv data] start (%d) ACCEPTED (out-of-order)\n", recvlen-8);
					}
					else{
						count = 0;
						for(i=la_seq; i < la_seq + WINDOW_LEN; i++){
							if(received[i]){
								count += 1;
							}
							else{
								break;
							}
						}
						if(count == 1){
							printf("[Recv data] start (%d) ACCEPTED (in-order)\n", recvlen-8);
						}
						else{
							printf("[Recv data] start (%d) ACCEPTED (out-of-order)\n", recvlen-8);
						}
					}
					ind = map_to_window(la_seq, seq_num);
					memcpy(windows[ind],buf+8,file_buf_size);
					window_lengths[ind] = file_buf_size;
					reverse_count = 0;
					//obtain a continous sequence, ready to write
					while(count > 0){
						fwrite(windows[reverse_count] , 1 , window_lengths[reverse_count], fp);
						received[(la_seq+reverse_count)%SEQ_LEN] = 0;//clear the status for each sequence id
						reverse_count++;
						count--;
					}
					if(ready_exit && last_window == la_seq + reverse_count-1) ready_exit = 2;
					la_seq = (la_seq + reverse_count)%SEQ_LEN;
				}
				else{
					printf("[Recv data] start (%d) IGNORED\n",recvlen-8);		
					/*if(!between(la_seq, (la_seq+WINDOW_LEN-1)%SEQ_LEN, seq_num)){
						//if ack got lost, resend it.
						packet = (char*)malloc(26*sizeof(char));
						*(short *) (packet+16) = (short) htons((la_seq-1)%SEQ_LEN);
						md5((uint8_t *) (packet+16), 10, (uint8_t *) packet);
						printf("[sending ack %d]\n", (la_seq-1)%SEQ_LEN); 
						if (sendto(fd, packet, 26, 0, (struct sockaddr *)&remaddr, addrlen) < 0)
							perror("sendto");
					}*/
				}
				/* sending back acknowledgement*/ 				
				*(u_short *) (packet+4) = (short) htons((la_seq-1)%SEQ_LEN);
				check_sum = calc_crc16(packet+2, 6);
				*(u_short *) (packet) = check_sum;
				printf("[sending ack %d]\n", (la_seq-1)%SEQ_LEN); 
				if (sendto(fd, packet, 8, 0, (struct sockaddr *)&remaddr, addrlen) < 0)
					perror("sendto");
				if(ready_exit == 2) break;
			}
				
		}
		else
			printf("uh oh - something went wrong!\n");	

	}
	//check whether ack is successfully transmitted
	fclose(fp);
	exit(0);
}

int map_to_window(int low, int ind){
	if(ind < low){
		return ind + (SEQ_LEN - low);
	}
	return ind - low;
}


// ./sendfile -r 128.42.208.6:18000 -f testfiles/30m.bigfile
// netsim --reorder 50 --drop 50 --mangle 50 --duplicate 50
