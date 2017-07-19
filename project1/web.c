#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<dirent.h>
#include<sys/stat.h>
#include<unistd.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include"web.h"
char* getDir(char* request_msg){
    const char s[2] = " ";
    char* token;
    token = strtok(request_msg , s);
    token = strtok(NULL, s);
    return token;
}

void getCmdDir(char** buffer, char* request_msg){
    const char s[2] = " ";
    buffer[0] = strtok(request_msg ,s);
    buffer[1] = strtok(NULL, s);
}

int validDir(char* request_dir){
    struct stat s;
    if (strstr(request_dir, "../")){
        return 0;
    }
    else if (stat(request_dir, &s) == 0){
        return 0;
    }
    else{
    return 1;
    }
}

int fileExist(char* request_dir){
    FILE* file;
    char full_dir[256];
    strcpy(full_dir, ".");
    strcat(full_dir, request_dir);
    file = fopen(full_dir,"r");
    if (file){
        fclose(file);
        return 1;
    }
    else{
        return 0;
    }
}

int Implemented(char* cmmd){
    const char* get = "GET";
    if ((strcmp(cmmd,get))==0){
    return 1;
    }
    return 0;
}
int serverIssue(char* request_dir){
    char full_dir[256];
    strcpy(full_dir, ".");
    strcat(full_dir, request_dir);
    if(!(access(full_dir, R_OK))){  
        return 0;
    }//server doesn't have read access or execute access to the file;
    return 1;
}

void sendingMsgs(int client_sock, char* err_msg, int msg_size){
    int cnt = 0;
	while(cnt < msg_size){
		cnt += send(client_sock, err_msg+cnt, msg_size-cnt, 0);
    } 
}

void sendingValidFile(int client_sock, char* file){
    FILE *fp;
    long lSize;
    char *buffer;
    char full_dir[256];
    strcpy(full_dir, ".");
    strcat(full_dir, file);

    fp = fopen(full_dir, "rb");

    fseek( fp, 0L , SEEK_END);
    lSize = ftell(fp);
    rewind(fp);

    /* allocate memory for entire content */
    buffer = calloc( 1, lSize+1 );
    if( !buffer ) fclose(fp),fputs("memory alloc fails",stderr),exit(1);

    /* copy the file into the buffer */
    if(1!=fread( buffer , lSize, 1 , fp))
          fclose(fp),free(buffer),fputs("entire read fails",stderr),exit(1);
    
    sendingMsgs(client_sock, buffer, lSize);
    fclose(fp);
    free(buffer);
}
void reportErr(int client_sock, char* err_msg, int mode){
    char header[256] = "HTTP/1.1 ";
    if(mode == 0){
        strcat(header, err_msg);
    }
    else{
        strcat(header, "200 OK");
    }
    strcat(header, " \r\n");
    sendingMsgs(client_sock, header, strlen(header));
    char* content_type = "Content-Type: text/html \r\n";
    sendingMsgs(client_sock, content_type, strlen(content_type));
    char* blank = "\r\n";
    sendingMsgs(client_sock, blank, strlen(blank));
    if (mode == 0){
        char html_body[256] = "<html><body><a>";
        strcat(html_body, err_msg);
        strcat(html_body,"</a></body></html>");
        sendingMsgs(client_sock, html_body, strlen(html_body));
    }
    else{
        sendingValidFile(client_sock,err_msg);
    }
}

void process(int client_sock, char* request_msg){
    char *cmmd_dir[2];
    getCmdDir(cmmd_dir, request_msg);
    //char* request_dir = getDir(request_msg);
    //int msg_size = sizeof(request_msg);
    char* cmmd = cmmd_dir[0];
    char* request_dir = cmmd_dir[1];
    char header_msg[256];
    if(!(Implemented(cmmd))){
        strcpy(header_msg, "501 Not Implemented");
        reportErr(client_sock, header_msg, 0);
        close(client_sock);

    }
    else if(!(validDir(request_dir))){
        strcpy(header_msg, "400 Bad Request");
        reportErr(client_sock, header_msg, 0);
        close(client_sock);
    }
    else if(!(fileExist(request_dir))){
        strcpy(header_msg,"404 Not Found");
        reportErr(client_sock,header_msg, 0);
        close(client_sock);
    }
    else if(serverIssue(request_dir)){
        strcpy(header_msg, "500 Internal Server Error");
        reportErr(client_sock, header_msg, 0);
        close(client_sock);
    }
    else{
        reportErr(client_sock, request_dir, 1);
        close(client_sock);
    }
}



