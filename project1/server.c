#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "util.h"
#include "web.h"

/*****************************************/
/* main program                          */
/*****************************************/

/* simple server, takes one parameter, the server port number */
int main(int argc, char **argv) {

    /* socket and option variables */
    int sock, new_sock, max;
    int optval = 1;

    /* server socket address variables */
    struct sockaddr_in sin, addr;
    unsigned short server_port = atoi(argv[1]);
    
    

    /* socket address variables for a connected client */
    socklen_t addr_len = sizeof(struct sockaddr_in);


    /* variables for select */
    fd_set read_set, write_set;
    struct timeval time_out;
    int select_retval;

    /* a silly message */
    //char *message = "Welcome! COMP/ELEC 429 Students!\n";

    /* number of bytes sent/received */
    //int count;

    /* numeric value received */
    //int num;

    /* linked list for keeping track of connected sockets */
    struct node head;
    struct node *current;
    //, *next;

    /* a buffer to read data */
    char *buf;
    int BUF_LEN = 1000;

    buf = (char *)malloc(BUF_LEN);

    /* initialize dummy head node of linked list */
    head.socket = -1;
    head.next = 0;

    char* req_dir;
    int mode_flag = 0; // mode_flag == 0 --> pingpong mode; 1 for www mode.
    if ((argc == 4) && (strcmp(argv[2], "www") == 0)) {
            req_dir = argv[3];
            mode_flag = 1;
            printf("%s", "you are in web mode\n");
    }
    /* now we keep waiting for incoming connections,
       check for incoming data to receive,
       check for ready socket to send more data */
    
    initServer(&sock, server_port, BACKLOG, &sin, optval);

    while (1)
    {

        /* set up the file descriptor bit map that select should be watching */
        FD_ZERO (&read_set); /* clear everything */
        FD_ZERO (&write_set); /* clear everything */

        FD_SET (sock, &read_set); /* put the listening socket in */
        max = sock; /* initialize max */

        /* put connected sockets into the read and write sets to monitor them */
        for (current = head.next; current; current = current->next) {
            FD_SET(current->socket, &read_set);

            if (current->pending_data) {
                /* there is data pending to be sent, monitor the socket
                   in the write set so we know when it is ready to take more
                   data */
                FD_SET(current->socket, &write_set);
            }

            if (current->socket > max) {
                /* update max if necessary */
                max = current->socket;
            }
        }

        time_out.tv_usec = 100000; /* 1-tenth of a second timeout */
        time_out.tv_sec = 0;

        /* invoke select, make sure to pass max+1 !!! */
        select_retval = select(max+1, &read_set, &write_set, NULL, &time_out);
        if (select_retval < 0)
        {
            perror ("select failed");
            abort ();
        }

        if (select_retval == 0)
        {
            /* no descriptor ready, timeout happened */
            continue;
        }

        if (select_retval > 0) /* at least one file descriptor is ready */
        {
            if (FD_ISSET(sock, &read_set)) /* check the server socket */
            {
                /* there is an incoming connection, try to accept it */
                new_sock = accept (sock, (struct sockaddr *) &addr, &addr_len);

                if (new_sock < 0)
                {
                    perror ("error accepting connection");
                    abort ();
                }

                if (mode_flag) {
                    char* recvbuffer;
                    recvbuffer = (char*)malloc(Msg_size_max);
                    memset(recvbuffer, 0, Msg_size_max);
                    recv(new_sock, recvbuffer, Msg_size_max, 0);
                    printf("%s\n", recvbuffer);

                    process(new_sock, recvbuffer);

                    free(recvbuffer);
                    close(new_sock);
                    continue;
                }
                /* make the socket non-blocking so send and recv will
                   return immediately if the socket is not ready.
                   this is important to ensure the server does not get
                   stuck when trying to send data to a socket that
                   has too much data to send already.
                   */
                if (fcntl (new_sock, F_SETFL, O_NONBLOCK) < 0)
                {
                    perror ("making socket non-blocking");
                    abort ();
                }
                
                /* the connection is made, everything is ready */
                /* let's see who's connecting to us */
                printf("Accepted connection. Client IP address is: %s\n",
                        inet_ntoa(addr.sin_addr));

                /* remember this client connection in our linked list */
                add(&head, new_sock, addr);

                /* let's send a message to the client just for fun */
                /*count = send(new_sock, message, strlen(message)+1, 0);
                if (count < 0)
                {
                    perror("error sending message to client");
                    abort();
                }*/
            }

            /* check other connected sockets, see if there is
               anything to read or some socket is ready to send
               more pending data */
            current = &head;
            //for (current = head.next; current; current = next) {
            while(current->next){
                current = current->next;
                int count;
                /* see if we can now do some previously unsuccessful writes */
                if (FD_ISSET(current->socket, &write_set)) {
                    /* the socket is now ready to take more data */
                    /* the socket data structure should have information
                       describing what data is supposed to be sent next.
                       but here for simplicity, let's say we are just
                       sending whatever is in the buffer buf
                       */
                    count = serverSending(current);
                    if (count <= 0) {
                        /* something is wrong */
                        if (count == 0) {
                            printf("Client closed connection. Client IP address is: %s\n", inet_ntoa(current->client_addr.sin_addr));
                        }   else {
                            perror("error receiving from a client");
                        }

                        /* connection is closed, clean up */
                        close(current->socket);
                        dump(&head, current->socket);
                    } 	      

                    /*if (count <= 0) {
                        if (errno == EAGAIN) {
                            we are trying to dump too much data down the socket,
                              it cannot take more for the time being 
                              will have to go back to select and wait til select
                              tells us the socket is ready for writing");
                            abort();
                        } else {
                             something else is wrong 
                            abort();
                        }
                    } */
                }
                /* note that it is important to check count for exactly
                   how many bytes were actually sent even when there are
                   no error. send() may send only a portion of the buffer
                   to be sent.
                   */

                if (FD_ISSET(current->socket, &read_set)) {
                    /* we have data from a client */
                    count = serverRecieving(current);
                    if (count <= 0) {
                        /* something is wrong */
                        if (count == 0) {
                            printf("Client closed connection. Client IP address is: %s\n", inet_ntoa(current->client_addr.sin_addr));
                        }   else {
                            perror("error receiving from a client");
                        }

                        /* connection is closed, clean up */
                        close(current->socket);
                        dump(&head, current->socket);
                    } 	      
                }
            }
        }
    }
}
