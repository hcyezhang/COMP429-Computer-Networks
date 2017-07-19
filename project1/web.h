#include<string.h>
#include<stdio.h>
#include<dirent.h>
#include<sys/stat.h>
#include<unistd.h>

char* getDir(char* request_msg);
void getCmdDir(char** buffer, char* request_msg);
int validDir(char* request_dir);
int fileExist(char* request_dir);
int Implemented(char* cmd);
int serverIssue(char* request_dir);
void sendingMsgs(int client_sock, char* err_msg, int msg_size);
void sendingValidFile(int client_sock, char* file);
void reportErr(int client_sock, char* err_msg, int mode);
void process(int client_sock, char* request_msg);
