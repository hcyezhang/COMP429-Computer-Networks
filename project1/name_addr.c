#include <stdio.h>
#include <arpa/inet.h>
#include <netdb.h>

/* Simple program that exercies
   gethostbyname(), gethostbyaddr(), inet_addr(), inet_ntoa()

   Input parameter: domain name (e.g. www.rice.edu)
*/
int main(int argc, char** argv) {

  char *hostname;
  unsigned int ipaddr_in_network_order;
  char *ipaddr_in_ascii; /* i.e. a.b.c.d format */
  struct hostent *gethostby_result;
  struct in_addr ipaddr_structure;

  hostname = argv[1];

  gethostby_result = gethostbyname(hostname);

  ipaddr_in_network_order = *(unsigned int *) gethostby_result->h_addr_list[0];

  ipaddr_structure.s_addr = ipaddr_in_network_order;

  ipaddr_in_ascii = inet_ntoa(ipaddr_structure);

  printf("IP address in dot format after manipulations: %s\n", ipaddr_in_ascii);

  ipaddr_in_network_order = inet_addr(ipaddr_in_ascii);

  gethostby_result = gethostbyaddr((char *)&ipaddr_in_network_order, sizeof(ipaddr_in_network_order), AF_INET);

  hostname = gethostby_result->h_name;

  printf("Host name after manipulations: %s\n", hostname);
  return 0;
}
