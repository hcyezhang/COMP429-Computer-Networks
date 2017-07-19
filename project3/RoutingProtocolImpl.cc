#include "RoutingProtocolImpl.h"

RoutingProtocolImpl::RoutingProtocolImpl(Node *n) : RoutingProtocol(n) {
  sys = n;
  // add your own code
  exp_alarm = new char[1];
  pp_alarm = new char[1];
  update_alarm = new char[1];
  exp_alarm[0] = 'e';
  pp_alarm[0] = 'p';
  update_alarm[0] = 'u';
}

RoutingProtocolImpl::~RoutingProtocolImpl() {
  // add your own code (if needed)
  delete[] exp_alarm;
  delete[] pp_alarm;
  delete[] update_alarm;
}

void RoutingProtocolImpl::init(unsigned short num_ports, unsigned short router_id, eProtocolType protocol_type) {
  // add your own code
  this->router_id = router_id;
  this->num_ports = num_ports; 
  this->protocol_type = protocol_type;
  //set up ports
  for(uint16_t i = 0; i < num_ports; i++){
    Port p = Port();
    p.to = 0;
    p.cost = 0;
    p.updated = 0;
    p.connected = false;
    ports[i] = p;  
  }
  generatePingPongMsg();
  sys->set_alarm(this, 10000, (void*) pp_alarm);
  sys->set_alarm(this, 30000, (void*) update_alarm);
  sys->set_alarm(this, 1000, (void*) exp_alarm);
}

void RoutingProtocolImpl::handle_alarm(void *data) {
  // add your own code
  char type = *(char *) data;
    if(type == 'e'){
        if(protocol_type == P_DV)
            refreshDV();
        sys->set_alarm(this, 1000, (void*) exp_alarm);
    }
    else if (type == 'p'){
        generatePingPongMsg();
        sys->set_alarm(this, 10000, (void*) pp_alarm);
    }
    else if (type == 'u'){
        if(protocol_type == P_DV){
            updateDVTable();
        }
        sys->set_alarm(this, 30000, (void*) update_alarm);
    }
    else{
        printf("Unknown alarm");
    }
}

void RoutingProtocolImpl::recv(unsigned short port, void *packet, unsigned short size) {
  // add your own code
    char type = *(char *) packet;
    if (type == DATA){
        recvDATA(port, packet, size);
    }
    else if (type == PING || type == PONG){
        handlePingPong(port, packet, size);
    }
    else if (type == DV){
        recvDV(port, packet, size);
    }
    else{
        printf("Unknown Error");
    }
}

void RoutingProtocolImpl::refreshDV(){
  //check port status
  std::vector<DVPair> new_packet;
  std::vector<uint16_t> to_delete;
  for(uint16_t i = 0; i < num_ports; i++){
    if(ports[i].connected && sys->time() - ports[i].updated > 15000){
      ports[i].connected = false;
      ports[i].cost = 0;//demo rest cost
      //direct_neighbors synchronize with port status
      if(direct_neighbors.find(ports[i].to) != direct_neighbors.end())
        direct_neighbors.erase(direct_neighbors.find(ports[i].to));
      //remove all entries in dv_map whose next_hop is ports.to
      for(DVMap::iterator it = dv_map.begin(); it != dv_map.end(); it++){
        if(it->second.next_hop == ports[i].to){
          to_delete.push_back(it->first);
        }
      }
    }
  }
  for(uint16_t i = 0; i < to_delete.size(); i++){
    //check if the dest is still in direct neighborhoods, change it 
    //to direct link
    if(direct_neighbors.find(to_delete[i]) != direct_neighbors.end()){
      dv_map[to_delete[i]].next_hop = to_delete[i];
      dv_map[to_delete[i]].cost = direct_neighbors[to_delete[i]].cost;
      dv_map[to_delete[i]].updated = sys->time();
      forwarding_table[to_delete[i]] = to_delete[i];//demo
      DVPair pair = std::make_pair(to_delete[i], dv_map[to_delete[i]].cost);
      new_packet.push_back(pair);
    }
    else{
      dv_map.erase(dv_map.find(to_delete[i]));
      forwarding_table.erase(forwarding_table.find(to_delete[i]));
      DVPair pair = std::make_pair(to_delete[i], INFINITY_COST);
      new_packet.push_back(pair);
    }
  }
  for(DVMap::iterator it = dv_map.begin(); it != dv_map.end(); ){
    if(sys->time() - it->second.updated > 45000){
      //check direct link
      if(direct_neighbors.find(it->first) != direct_neighbors.end()){
        it->second.next_hop = it->first;
        it->second.cost = direct_neighbors[it->first].cost;
        it->second.updated = sys->time();
        //forwarding_table[it->first] = direct_neighbors[it->second.next_hop].port;
        forwarding_table[it->first] = it->first;
        DVPair pair = std::make_pair(it->first, it->second.cost);
        new_packet.push_back(pair);
        it++;
      }
      else{
        DVPair pair = std::make_pair(it->first, INFINITY_COST);
        new_packet.push_back(pair);
        forwarding_table.erase(forwarding_table.find(it->first));
        it = dv_map.erase(it);
      }
    }
    else it++;
  }
  if(new_packet.size()){
    sendDVUpdate(new_packet);
  }
}

//check 

// add more of your own code
