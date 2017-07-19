CC = g++
COPTS = -g -Wall -std=c++11
LKOPTS = 

OBJS =\
	Event.o\
	Link.o\
	Node.o\
	RoutingProtocolImpl.o\
	Simulator.o\
	DVImpl.o\
	FwdDataImpl.o\
	PingPongImpl.o

HEADRES =\
	global.h\
	Event.h\
	Link.h\
	Node.h\
	RoutingProtocol.h\
	Simulator.h

%.o: %.cc
	$(CC) $(COPTS) -c $< -o $@

all:	Simulator

Simulator: $(OBJS)
	$(CC) $(LKOPTS) -o Simulator $(OBJS)

$(OBJS): global.h
Event.o: Event.h Link.h Node.h Simulator.h
Link.o: Event.h Link.h Node.h Simulator.h
Node.o: Event.h Link.h Node.h Simulator.h
Simulator.o: Event.h Link.h Node.h RoutingProtocol.h Simulator.h 
RoutingProtocolImpl.o: RoutingProtocolImpl.h 
DVImpl.o: RoutingProtocolImpl.h
FwdDataImpl.o: RoutingProtocolImpl.h
PingPongImpl.o: RoutingProtocolImpl.h
clean:
	rm -f *.o Simulator

