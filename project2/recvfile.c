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
#include "util.h"
#include "fix_queue.h"

#include <libgen.h>
#include <stdlib.h>

int main(int argc, char **argv){
    int i;
    struct sockaddr_in sin;	/* our address */
    struct sockaddr_in remaddr;	/* remote address */
    socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
    int recvlen;			/* # bytes received */
    int fd;				/* our socket */
    char buf[BUFLEN+16];	/* receive buffer */
    char* packet;			/* sending the ack packet */
    unsigned short server_port = atoi(argv[2]);
    char file_name[1024];
    FILE *fp;			/* pointer to the destination file */

    struct timeval tv;


    char flag[] = "-p";
    if(strcmp(argv[1],flag) != 0){
        perror("Invalid flag\n");
        return 0;
    }


    /* create a UDP socket */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket\n");
        exit(1);
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

    //char windows[WINDOW_LEN][BUFLEN];
    int la_seq = 0;
    int received[SEQ_LEN];
    int count = 0;
    int reverse_count = 0;
    int ind;
    for(i=0;i<SEQ_LEN;i++){
        received[i]=0;
    }

    u_short check_sum = 0;
    packet = (char*)malloc(16*sizeof(char));

    while(1){
        recvlen = recvfrom(fd, buf, BUFLEN, 0, (struct sockaddr *)&remaddr, &addrlen);
        if (recvlen > 0){
            check_sum = calc_crc16(buf+2, recvlen-2);
            if(ntohs(*(u_short *)(buf)) != check_sum){
                printf("[recv corrupt packet]\n");
                continue;
            }
            else{
                //copy buf to file_name
                gettimeofday(&tv,NULL);
                uint32_t ack_sec = tv.tv_sec;
                uint32_t ack_usec = tv.tv_usec;
                memcpy(file_name, buf+8+4, recvlen-8-4);
                *(u_short *) (packet+2) = htons(0);//seq num
                *(u_short *) (packet+4) = htons(SEQ_LEN-1);//ack num
                *(u_short *) (packet+6) = htons(1);//ack type
                *(uint32_t *) (packet+8) = htonl(ack_sec);
                *(uint32_t *) (packet+12) = htonl(ack_usec);
                check_sum = calc_crc16(packet+2, 14);
                *(u_short *) (packet) = htons(check_sum);
                //printf("[sending ack %d]\n", SEQ_LEN-1); 
                if (sendto(fd, packet, 16, 0, (struct sockaddr *)&remaddr, addrlen) < 0)
                    perror("sendto");
                break;
            }
        }
    }

    /* now loop, receiving data and write the payload into a given file */
    printf("waiting on port %d\n", server_port);

    char* part_1 = strdup(file_name);
    char* part_2 = strdup(file_name);

    //Make Directory.
    char sub_dir[250];
    char* tmp_sub_dir = dirname(part_1);
    char* filename = basename(part_2);
    memcpy(sub_dir, tmp_sub_dir, strlen(tmp_sub_dir)+1); 
    char mkcmd_mkdir[250];
    sprintf(mkcmd_mkdir, "mkdir -p %s", sub_dir);
    system(mkcmd_mkdir);

    //Get the filename and append .recv to it.
    const char delim[2] = ".";
    char name[250];

    char* tmp_name = strtok(filename,delim);
    memcpy(name, tmp_name, strlen(tmp_name)+1);
    strcat(name, ".recv");	
    strcat(sub_dir, "/");
    strcat(sub_dir, name);
    printf("write to %s\n", sub_dir);

    fp = fopen(sub_dir, "w");
    if(fp == NULL){
        perror("open file error");
        exit(1);
    }
    int ready_exit = 0;
    int last_window = -1;
    uint32_t last_id = 0;
    uint32_t packet_id;
    Node* root = NULL;
    for(i = 0; i < WINDOW_LEN; i++){
        Node * node_id = (Node*)malloc(sizeof(Node));
        node_id->data = (char*)malloc((BUFLEN+12+4)*sizeof(char));
        node_id->next = NULL;
        Enqueue(&root, node_id);
    }
    int total_count = 0;
    while(1) {
        recvlen = recvfrom(fd, buf, BUFLEN+12+4, MSG_DONTWAIT, (struct sockaddr *)&remaddr, &addrlen);
        if (recvlen > 0) {
            //buf[recvlen] = 0;
            /* check the correctness of the packet using crc16 */
            check_sum = calc_crc16(buf+2, recvlen-2);
            if (ntohs(*(u_short *)buf) != check_sum) {
                printf("[recv corrupt packet]\n");
                //continue;
            }
            else{
                u_short type = ntohs(*(u_short *)(buf+6));
                packet_id = ntohl(*(uint32_t *)(buf+8));
                //printf("packet_id, %d, last_id %d\n", packet_id, last_id);

                if((type == 2 || type == 4) && packet_id > last_id){
                    u_short seq_num = ntohs(*(u_short*)(buf+2));
                    if(type == 4) {
                        ready_exit = 1;
                        last_window = seq_num;
                    }

                    /* Should be Accepted!*/
                    if(between(la_seq, (la_seq+WINDOW_LEN-1)%SEQ_LEN, seq_num)&& !received[seq_num]) {
                        int file_buf_size = recvlen-8-4; /* the received buffer size minus the headers' size */					
                        received[seq_num] = 1;
                        if (seq_num != la_seq){
                            count = 0;
                            printf("[Recv data] start (%d) ACCEPTED (out-of-order)\n", recvlen-8-4);
                        }
                        else{
                            count = 0;
                            for(i=la_seq; i < la_seq + WINDOW_LEN; i++){
                                if(received[i%SEQ_LEN]){//demo
                                    count += 1;
                                }
                                else{
                                    break;
                                }
                            }
                            if(count == 1){
                                printf("[Recv data] start (%d) ACCEPTED (in-order)\n", recvlen-8-4);
                            }
                            else{
                                printf("[Recv data] start (%d) ACCEPTED (out-of-order)\n", recvlen-8-4);
                            }
                        }
                        ind = map_to_window(la_seq, seq_num);
                        Node* node_id = GetK(root, ind);
                        if(node_id == NULL){
                            node_id = (Node*)malloc(sizeof(Node));
                            node_id->data = (char*)malloc((BUFLEN+12+4)*sizeof(char));
                            node_id->next = NULL;
                            Enqueue(&root, node_id);
                        }
                        memcpy(node_id->data,buf+8+4,file_buf_size);
                        node_id->size = file_buf_size;
                        reverse_count = 0;
                        total_count += count;
                        //if(count > 0) printf("count %d\n", total_count);
                        //obtain a continous sequence, ready to write
                        while(count > 0){
                            last_id++;
                            int write_len = fwrite(root->data, sizeof(char), root->size, fp);//to demo
                            //printf("write len %d %d\n", write_len, root->size);
                            Dequeue(&root);
                            node_id = (Node*)malloc(sizeof(Node));
                            node_id->data = (char*)malloc((BUFLEN+12+4)*sizeof(char));
                            node_id->next = NULL;
                            Enqueue(&root, node_id);
                            received[(la_seq+reverse_count)%SEQ_LEN] = 0;//clear the status for each sequence id
                            reverse_count++;
                            count--;
                        }
                        if(ready_exit && last_window == la_seq + reverse_count-1) ready_exit = 2;
                        la_seq = (la_seq + reverse_count)%SEQ_LEN;
                    }
                    else{
                        printf("[recv data] start (%d) IGNORED\n",recvlen-8-4);		
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
                }
            }
        }
        /* sending back acknowledgement*/
        gettimeofday(&tv,NULL);
        *(u_short *) (packet+2) = htons(0);
        *(u_short *) (packet+4) = htons(mod(la_seq-1, SEQ_LEN));
        *(u_short *) (packet+6) = htons(1);
        *(uint32_t *) (packet+8) = htonl(tv.tv_sec);
        *(uint32_t *)(packet+12) = htonl(tv.tv_usec);
        check_sum = calc_crc16(packet+2, 14);
        *(u_short *) (packet) = htons(check_sum);
        //printf("[sending ack %d]\n", mod(la_seq-1, SEQ_LEN)); 
        if (sendto(fd, packet, 16, 0, (struct sockaddr *)&remaddr, addrlen) < 0)
            perror("sendto");
        if(ready_exit == 2) break;
        //}

        //}

    }
//check whether ack is successfully transmitted

fclose(fp);
count = 400;
while(count--){
    gettimeofday(&tv,NULL);
    *(u_short *) (packet+2) = htons(0);
    *(u_short *) (packet+4) = htons(mod(la_seq-1, SEQ_LEN));
    *(u_short *) (packet+6) = htons(1);
    *(uint32_t *) (packet+8) = htonl(tv.tv_sec);
    *(uint32_t *)(packet+12) = htonl(tv.tv_usec);
    check_sum = calc_crc16(packet+2, 14);
    *(u_short *) (packet) = htons(check_sum);
    //printf("[sending ack %d]\n", mod(la_seq-1, SEQ_LEN)); 
    if (sendto(fd, packet, 16, 0, (struct sockaddr *)&remaddr, addrlen) < 0)
        perror("sendto");
}
printf("[Completed!]\n");
free(packet);
while(root){
    Dequeue(&root);
}
exit(0);
}


