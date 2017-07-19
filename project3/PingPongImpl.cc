#include "RoutingProtocolImpl.h"
void RoutingProtocolImpl::generatePingPongMsg(){
    for (uint16_t i = 0 ; i < num_ports; i++){
        char * pingpkt = (char*)malloc(12*sizeof(char));
        *pingpkt = PING;
        *((uint16_t *)pingpkt + 1) = htons(12);
        *((uint16_t *)pingpkt + 2) = htons(router_id);
        *((uint32_t *)pingpkt + 2) = htonl(sys->time());
        sys->send(i, pingpkt, 12);
    }
}

void RoutingProtocolImpl::handlePingPong(uint16_t port, void *packet, uint16_t size){
    //Directly sendback a pong message, update the srcID to your own id and fill the destination id with srcID
    char type = *((char*) packet);
    if (type == PING){
        char * pongpkt = (char *) packet;
        *pongpkt = PONG;
        *((uint16_t *)pongpkt + 3) = htons(ntohs(*((uint16_t *)pongpkt + 2)));
        *((uint16_t *)pongpkt + 2) = htons(router_id);
        sys->send(port, pongpkt, 12);
    }
    else if (type == PONG){
        uint32_t currentTime = sys->time();
        uint32_t rrt = currentTime - ntohl(*(( uint32_t* )packet + 2));
        
        //1. Update ports table
        uint16_t srcID = ntohs(*((uint16_t *)packet + 2)); 
        //uint16_t destID = ntohs(*((uint16_t *) packet +4));
        ports[port].to = srcID;
        bool disconnected = !ports[port].connected;
        ports[port].connected = true;
        ports[port].updated = currentTime;//update the time
        if(direct_neighbors.find(srcID) != direct_neighbors.end() ){
            direct_neighbors[srcID].port = port;
            direct_neighbors[srcID].cost = rrt;
        }else{
            NNCell new_cell;
            new_cell.port = port;
            new_cell.cost = rrt;
            direct_neighbors[srcID] = new_cell;
        }
        
        uint32_t old_rrt = ports[port].cost;
        std::vector<DVPair> new_packet;
        //scan throught the port list to make sure no other port connects to this srcID;
        for(auto it = ports.begin(); it != ports.end(); it++){
            if(it->first != port && it->second.to == srcID){
                it->second.connected = false;
            }
        }
        
        //2. process a pong message - needs to update the port table and dv table
        ports[port].cost = rrt;
        uint32_t diff = rrt - old_rrt;
        

        if(diff != 0){
            for (auto it = dv_map.begin(); it!=dv_map.end(); ++it){
                if (it->second.next_hop == srcID){
                    uint16_t maychange_cost = it->second.cost + diff;
                    if(direct_neighbors.find(it->first) != direct_neighbors.end()){
                        if (direct_neighbors[it->first].cost <= maychange_cost){
                            it->second.next_hop = it->first;
                            it->second.cost = direct_neighbors[it->first].cost;
                            //update the forwarding table
                            forwarding_table[it->first] = it->first;//demo
                        }
                    }
                    else{
                        it->second.cost = maychange_cost;
                    }
                    it->second.updated = sys->time();
                    DVPair dv_updated = std::make_pair(it->first, it->second.cost);
                    new_packet.push_back(dv_updated);
                }
                //if the direct link is faster
                else if(it->first == srcID && rrt <= dv_map[it->first].cost){//demo
                    it->second.next_hop = srcID;
                    it->second.cost = rrt;
                    forwarding_table[it->first] = srcID;
                    it->second.updated = sys->time();
                    DVPair dv_updated = std::make_pair(it->first, it->second.cost);
                    new_packet.push_back(dv_updated);
                }
                //send more information if disconnected before
                else if(disconnected && dv_map.find(srcID) != dv_map.end()){
                    DVPair dv_updated = std::make_pair(it->first, it->second.cost);
                    new_packet.push_back(dv_updated);
                }
            }
        }
        //only need to change update time if rrt doesn't change
        else{
            for(auto it = dv_map.begin(); it != dv_map.end(); it++){
                if(it->second.next_hop == srcID || it->first == srcID){
                    it->second.updated = sys->time();
                }
            }
        }

        if(dv_map.find(srcID) == dv_map.end()){
            DVCell new_entry;
            new_entry.next_hop = srcID; 
            new_entry.cost = rrt;
            new_entry.updated = sys->time();
            dv_map[srcID] = new_entry;
            forwarding_table[srcID] = srcID; 
            //generate new dv packet;
            for (auto it = dv_map.begin(); it != dv_map.end(); ++it){
                DVPair dv_updated = std::make_pair(it->first,it->second.cost); 
                new_packet.push_back(dv_updated);
            }
        }
        if(new_packet.size()){
            sendDVUpdate(new_packet);
        }
    }
}


