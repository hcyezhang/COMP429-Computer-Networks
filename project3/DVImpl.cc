#include "RoutingProtocolImpl.h"

void RoutingProtocolImpl::recvDV(uint16_t port, void *packet, uint16_t size){
  char *pkt = (char *) packet;
  //get the id of source router
  uint16_t source_id = ntohs(*((uint16_t *)pkt + 2));
  uint16_t *costs = (uint16_t *)packet + 4;
  //find the cost for path this->source
  auto it_source = direct_neighbors.find(source_id);
  if(it_source == direct_neighbors.end()){
    NNCell n_cell;
    n_cell.port = port;
    for(uint16_t i = 0; i < size/2 - 4; i += 2){
      uint16_t dest_id = ntohs(*(costs + i));
      uint16_t cost = ntohs(*(costs + i + 1));
      if(dest_id == router_id){
        n_cell.cost = cost;
        break;
      }
    }
    direct_neighbors[source_id] = n_cell;
    it_source = direct_neighbors.find(source_id);
  }
  //load neigbhor id and cost
  //and make a new packet for updating
  std::vector<DVPair> new_packet;
  for(uint16_t i = 0; i < size/2 - 4; i += 2){
    uint16_t dest_id = ntohs(*(costs + i));
    uint16_t cost = ntohs(*(costs + i + 1));
    //special attention for poision reverse
    //also pay attention to direct link
    DVMap::iterator it;
    //find the current best route to dest in dv_map
    it = dv_map.find(dest_id);
    if(it != dv_map.end()){
      //update dv table upon finding new minimum to dest d(source, dest) + d(this, source)
      //or source == next_hop, then we need to update the path with new d(source, dest)
      if(cost != INFINITY_COST){
        if(it->second.cost > cost + it_source->second.cost) {
          it->second.cost = cost + it_source->second.cost;
          it->second.next_hop = source_id;
          it->second.updated = sys->time();
          DVPair pair = std::make_pair(dest_id, it->second.cost);
          //add this pair to update packet
          new_packet.push_back(pair);
          //update forwarding table
          forwarding_table[dest_id] = source_id;
        }
        //if d(source, dest) is changed, check whether we have a direct link 
        //between this and dest, choose the smallest one between two paths
        else if(source_id == it->second.next_hop){
          if(direct_neighbors.find(dest_id) == direct_neighbors.end() || 
              direct_neighbors[dest_id].cost > cost + it_source->second.cost){
            it->second.cost = cost + it_source->second.cost;
            it->second.next_hop = source_id;
            it->second.updated = sys->time();
            DVPair pair = std::make_pair(dest_id, it->second.cost);
            new_packet.push_back(pair);
            forwarding_table[dest_id] = source_id;
          }
          //the direct link is smaller than updated path
          else{
            it->second.cost = direct_neighbors[dest_id].cost;
            it->second.next_hop = dest_id;
            it->second.updated = sys->time();
            DVPair pair = std::make_pair(dest_id, it->second.cost);
            new_packet.push_back(pair);
            forwarding_table[dest_id] = dest_id;
          }
        }
      }
      //handle poison reverse
      else{
        //direct links are stored in direct_neighbors
        if(source_id == it->second.next_hop){
          //though the link source -> dest is broken, we have direct link this->dest
          if(direct_neighbors.find(dest_id) != direct_neighbors.end()){
            it->second.cost = direct_neighbors[dest_id].cost;
            it->second.next_hop = dest_id;
            it->second.updated = sys->time();
            forwarding_table[dest_id] = dest_id;
            DVPair pair = std::make_pair(dest_id, it->second.cost);
            new_packet.push_back(pair);
          }   
          else{
            //this->dest link is broken, remove it and inform neighbors
            dv_map.erase(it);
            forwarding_table.erase(forwarding_table.find(dest_id));
            DVPair pair = std::make_pair(dest_id, INFINITY_COST);
            new_packet.push_back(pair);
          }
        }
        //tell neighbors his path
        else{
          DVPair pair = std::make_pair(dest_id, it->second.cost);
          new_packet.push_back(pair);
          new_packet.push_back(pair);
        }
      }
    }  
    //find a new path to a new dest or this is a direct link
    else{
      if(cost != INFINITY_COST && dest_id != router_id){
        DVCell new_cell;
        new_cell.next_hop = source_id;
        new_cell.cost = cost + it_source->second.cost;
        new_cell.updated = sys->time();
        dv_map[dest_id] = new_cell;
        DVPair pair = std::make_pair(dest_id, new_cell.cost);
        new_packet.push_back(pair);
        //update forwarding table
        forwarding_table[dest_id] = source_id;
      }
      //this is a direct link, skip it cause direct link are updated via 
      //ping pong
      /*else if( cost != INFINITY_COST ){
        //a whole new direct link
        if(dv_map.find(source_id) == dv_map.end()){
          DVCell new_cell;
          new_cell.next_hop = source_id;
          new_cell.cost = cost;
          new_cell.updated = sys->time();
          dv_map[source_id] = new_cell;
          //DVPair pair = std::make_pair(source_id, new_cell.cost);
          direct_neighbors[source_id].cost = cost;
          //we don't need to inform the changes about direct link
          //new_packet.push_back(pair);
          forwarding_table[source_id] = source_id;
        }
        //update existing direct link
        else if(dv_map[source_id].cost >= cost){
          dv_map[source_id].cost = cost;
          dv_map[source_id].next_hop = source_id;
          dv_map[source_id].updated = sys->time();
          //DVPair pair = std::make_pair(source_id, cost);
          direct_neighbors[source_id].cost = cost;
          //new_packet.push_back(pair);
          forwarding_table[source_id] = source_id;
        }
      }*/
    }
  }
  //send updated distance vector to neighborhoods
  if(new_packet.size()){
    sendDVUpdate(new_packet);
  }
}

void RoutingProtocolImpl::sendDVUpdate(std::vector<DVPair> const & new_packet){
  uint16_t dv_table_size = new_packet.size()*4 + 8;
  for(uint16_t i = 0; i < num_ports; i++){
    if(ports[i].connected){
      char * new_dv_table = (char *) malloc(sizeof(char)*dv_table_size);
      *new_dv_table = DV;
      *((uint16_t *) new_dv_table + 1) = htons(dv_table_size);
      *((uint16_t *) new_dv_table + 2) = htons(router_id);
      *((uint16_t *) new_dv_table + 3) = htons(ports[i].to);
      uint16_t count = 4;
      for(uint16_t j = 0; j < new_packet.size(); j++){
        *((uint16_t *) new_dv_table + count) = htons(new_packet[j].first);
        count++;
        *((uint16_t *) new_dv_table + count) = htons(new_packet[j].second);
        count++;
      }
      sys->send(i, new_dv_table, dv_table_size);
    }
  }
}

void RoutingProtocolImpl::updateDVTable(){
  std::vector<DVPair> new_packet;
  for(DVMap::iterator it = dv_map.begin(); it != dv_map.end(); it++){
    DVPair pair = std::make_pair(it->first, it->second.cost);
    new_packet.push_back(pair);
  }
  sendDVUpdate(new_packet);
}
