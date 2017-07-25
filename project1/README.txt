COMP 556 Project 1:
Ye Zhang
Sep 21 2016

This project contains a “ping-pong” client program and a server program. 
The “ping-pong” client and the server together are used to measure the network’s performance in transmitting messages of various sizes. 
In addition to responding to the “ping-pong” client messages, the server has an alternate execution mode in which it acts as a simple web server.

Specifically, the client program takes 4 command line parameters, in the following order:
- hostname
- port
- size (the size in bytes of each message to send)
- count (the number of message exchanges to perform)

the server takes 1 mandatory parameter and 2 optional parameters, in the following order
- port
- mode(optional) When the server’s optional mode parameter is set to “www”, the server runs in web server mode.  
- root_directory(required for www mode)

———————————————————————————————————————————-
Please use the Makefile to compile the code.

Ping Pong mode testing:
1 set up server: ./server server_portnumber
2 set up client: ./client host_name server_portnumber size_of_msgs num_of_msgs
3 The average latency should be printed on the screen.

www mode testing:
1 setup server: ./server server_portnumber www .
2 setup client: type the following command in the browser:
3 Either the error message or file content should be visualized in your browser.

My method for measuring the non-bandwidth dependent network latency is reported in part3.pdf. 
