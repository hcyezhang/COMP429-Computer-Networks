CC	 	= gcc
LD	 	= gcc
CFLAGS	 	= -Wall -g
LDFLAGS	 	= 
DEFS 	 	=

all: server client name_addr

server: server.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o server server.c util.c web.c

client: client.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o client client.c util.c

name_addr:	name_addr.c
	$(CC) $(DEFS) $(CFLAGS) $(LIB) -o name_addr name_addr.c
clean:
	rm -f *.o
	rm -f *~
	rm -f core.*.*
	rm -f server
	rm -f client
	rm -f name_addr
