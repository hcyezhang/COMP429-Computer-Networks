#include "RoutingProtocolImpl.h"

void RoutingProtocolImpl::recvDATA(uint16_t port, void *packet, uint16_t size){
  ePacketType type = (ePacketType)(*(char *)packet);
  //uint16_t source_id = ntohs(*((uint16_t *) packet + 2));
  uint16_t dest_id = ntohs(*((uint16_t *) packet + 3));
  if(type == DATA){
    if(dest_id == router_id){
      //printf("\tReceived data from %d.\n", source_id);
      delete[] (char *)packet;
    }
    else{
      if(forwarding_table.find(dest_id) == forwarding_table.end()){
        //printf("\tUnreachable destination: %d.\n", dest_id);
        return;
      }
      else{
        //if(port == SPECIAL_PORT) 
          //printf("\tSend Data generated at %d to %d via %d.\n", router_id, dest_id, forwarding_table[dest_id]);
        //else
          //printf("\tTransfer Data to %d via %d.\n", dest_id, forwarding_table[dest_id]);
        sys->send(direct_neighbors[forwarding_table[dest_id]].port, packet, size);
      }
    }
  }
  else
    printf("\tUnknown packet type.\n");
}
