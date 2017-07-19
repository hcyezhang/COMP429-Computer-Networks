#include <netinet/in.h>
#include <sys/time.h>
#include <math.h>
extern int Client_arg;
extern unsigned short Server_port_max;
extern unsigned short Server_port_min;
extern int Msg_size_min;
extern int Msg_size_max;
extern int Block_size_min;
extern int Block_size_max;

  /* maximum number of pending connection requests */
extern int BACKLOG;

  /* A linked list node data structure to maintain application
   information related to a connected socket */
struct node {
  int socket;
  struct sockaddr_in client_addr;
  int pending_data; /* flag to indicate whether there is more data to send */
  /* you will need to introduce some variables here to record
     all the information regarding this socket.
     e.g. what data needs to be sent next */
  char* buffer;
  int added_pos;
  int len_buffer;
  struct node *next;
};
void dump(struct node* head, int socket);
void add(struct node* head, int socket, struct sockaddr_in addr);
void setTimeStamp(char* sendbuffer, struct timeval* tv);
int validateUserInput(int argc, unsigned short server_port, unsigned short size_msg, int count);
void initServer(int* sock,unsigned short server_port, int BACKLOG, struct sockaddr_in* psin, int);
int serverSending(struct node* current);
int serverRecieving(struct node* current);
void initClient(char** pbuffer, char** psendbuffer, unsigned short size_msg, int* psock, struct sockaddr_in* psin, unsigned short  server_port, unsigned int server_addr);

