#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/time.h>


#define PACKET_SIZE 5000

int main(int argc, char** argv){
    struct sockaddr_in sin;
    int sock; 
    
    char* server_info = argv[2];
    char* dir_file = argv[4]; 
    char sub_dir[64];
    char file_name[64];
    char host_name[64];
    int server_port;
    
    struct timeval tv;
    if (sscanf(server_info, "%[^:]:%d", host_name, &server_port) != 2){
        perror("Wrong Server Arguments!");
        return 0;
    }
    if(sscanf(dir_file, "%[^:]\/%s", sub_dir, file_name) != 2){
        perror("Wrong Directory Arguments!");
        return 0;
    }
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        printf("Opening UDP socket");
    }    
    
    struct hostent *host = gethostbyname(host_name);
    unsigned int server_addr = *(unsigned int *) host->h_addr_list[0];
    
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = server_addr;
    sin.sin_port = htons(server_port);
    //socklen_t sin_len = sizeof(sin);
    char* packet;
    packet = (char*)malloc(PACKET_SIZE*sizeof(char));
    gettimeofday(&tv,NULL);
    *(unsigned short *) packet = chksum(sub_dir,strlen(sub_dir));
    *(int *) packet = 
    
    strcpy(packet + 16,sub_dir);
    
    gettimeofday(&tv, NULL);
    sendto(sock, packet, 16 + strlen(sub_dir), 0, (struct sockaddr *)&sin, slen); 
    return 0;
}
