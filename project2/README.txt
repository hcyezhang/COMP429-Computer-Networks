In this project, I designed a simple transport protocol that provides reliable file transfer. The protocol is responsible for ensuring data is delivered in order, without duplicates, missing data, or errors. The protocol is implemented using SOCK_DGRAM sockets(which use the UDP unreliable datagram protocol) to transmit and receive packets.

The project contains two programs:
a sendfile program sends the file across the network
a recvfile program receives the file and stores it to local disk.