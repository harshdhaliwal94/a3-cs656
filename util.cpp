#include "util.h"

unsigned char * serialize_unsigned_int(unsigned char *buffer, int value)
{
  /* Write little-endian int value into buffer; assumes 32-bit int and 8-bit char. */
        if(sizeof(unsigned int)!=4){
                perror("Error: expecting unsigned int size to be of 32 bits");
                exit(EXIT_FAILURE);
        }
        buffer[0] = value & 0xFF;
        buffer[1] = (value >> 8) & 0xFF;
        buffer[2] = (value >> 16) & 0xFF;
        buffer[3] = (value >> 24) & 0xFF;
        return buffer + 4;
}

unsigned char * serialize_char(unsigned char *buffer, char value)
{
        if(sizeof(char)!=1){
                perror("Error: expecting char size to be of 8 bits");
                exit(EXIT_FAILURE);
        }
        buffer[0] = value;
        return buffer + 1;
}

unsigned char * serialize_pkt_INIT(unsigned char *buffer, const struct pkt_INIT *init_pkt)
{
  buffer = serialize_unsigned_int(buffer, init_pkt->router_id);
  return buffer;
}
unsigned char * serialize_pkt_HELLO(unsigned char *buffer, const struct pkt_HELLO *hello_pkt){
    buffer = serialize_unsigned_int(buffer, hello_pkt->router_id);
    buffer = serialize_unsigned_int(buffer, hello_pkt->link_id);
    return buffer;
}

unsigned char * serialize_pkt_LSPDU(unsigned char *buffer, const struct pkt_LSPDU *lspdu_pkt){
    buffer = serialize_unsigned_int(buffer, lspdu_pkt->sender);
    buffer = serialize_unsigned_int(buffer, lspdu_pkt->router_id);
    buffer = serialize_unsigned_int(buffer, lspdu_pkt->link_id);
    buffer = serialize_unsigned_int(buffer, lspdu_pkt->cost);
    buffer = serialize_unsigned_int(buffer, lspdu_pkt->via);
    return buffer;
}


void deserialize_circuit_DB(char * buffer, struct circuit_DB* circuit, int size, int num_bytes){
	if(num_bytes!=size){
		perror("Error: did not recieve correct number of bytes for circuit_DB");
		exit(EXIT_FAILURE);
    }
    memcpy ( circuit, buffer, num_bytes );
}

int send_pkt_INIT(int socket, const struct sockaddr *dest, socklen_t dlen, const struct pkt_INIT *init_pkt, FILE* logfd)
{
  unsigned char buffer[32] = {0}; 
  unsigned char *ptr;
  ptr = serialize_pkt_INIT(buffer, init_pkt);
  fprintf(logfd, "R%u sends an INIT: router_id %u\n", init_pkt->router_id, init_pkt->router_id);
  return sendto(socket, buffer, ptr - buffer, MSG_CONFIRM, dest, dlen) != ptr - buffer;
}

int hostname_to_ip(char * hostname , char* ip)
{
        struct hostent *he;
        struct in_addr **addr_list;
        int i;

        if ( (he = gethostbyname( hostname ) ) == NULL) 
        {
                // get the host info
                herror("gethostbyname");
                return 1;
        }

        addr_list = (struct in_addr **) he->h_addr_list;

        for(i = 0; addr_list[i] != NULL; i++) 
        {
                //Return the first one;
                strcpy(ip , inet_ntoa(*addr_list[i]) );
                return 0;
        }

        return 1;
}

int send_hello_all(int socket, const struct sockaddr * dest, socklen_t dlen, struct circuit_DB* circuit, int router_id,FILE* logfd){
    int i =0;
    for(i=0;i<circuit->nbr_link;i++){
        //create hello pdu
        unsigned char buffer[2*32] = {0};
        unsigned char *ptr;
        struct pkt_HELLO hello_pkt = {(unsigned int)router_id,(unsigned int)circuit->linkcost[i].link};
        ptr = serialize_pkt_HELLO(buffer, (const struct pkt_HELLO*)&hello_pkt);
        fprintf(logfd, "R%d sends a HELLO: router_id %u link_id %u\n",router_id,hello_pkt.router_id,hello_pkt.link_id);
        if(sendto(socket, buffer, ptr - buffer, MSG_CONFIRM, dest, dlen) != ptr - buffer){
            return 1;
        }
    }
    return 0;
}

int reply_hello(int socket, const struct sockaddr * dest, socklen_t dlen, struct circuit_DB* ls_DB, int router_id, struct pkt_HELLO* pkt_hello,FILE* logfd){
    unsigned int sender = (unsigned int)router_id;
    unsigned int via = pkt_hello->link_id;
    unsigned int reciever = pkt_hello->router_id;
    for(int i=0;i<NBR_ROUTER;i++){
        if(i==(int)(reciever-1)){continue;}
        for(int j=0;j<ls_DB[i].nbr_link;j++){
            unsigned char buffer[5*32] = {0};
            unsigned char *ptr;
            struct pkt_LSPDU pkt_lspdu = {sender,(unsigned int)(i+1),ls_DB[i].linkcost[j].link,ls_DB[i].linkcost[j].cost,via};
            ptr = serialize_pkt_LSPDU(buffer,(const struct pkt_LSPDU*)&pkt_lspdu);
            fprintf(logfd, "R%d sends an LSPDU: sender %u router_id %u link_id %u cost %u via %u\n",router_id, sender,pkt_lspdu.router_id, pkt_lspdu.link_id,pkt_lspdu.cost,pkt_lspdu.via);
            if(sendto(socket, buffer, ptr - buffer, MSG_CONFIRM, dest, dlen) != ptr - buffer){
                return 1;
            }
        }
    }
    return 0;
}

bool unique(unsigned int router_id, unsigned int link_id, unsigned int cost, map &mymap, link_map &link2router, pair adj_matrix[][5]){
    std::pair<map::iterator,bool> ret;
    std::pair<link_map::iterator,bool> ret1;
    ret = mymap.insert ( std::pair<pair,int>(std::make_pair(router_id,link_id),cost) );
    if (ret.second==false) {
        return false;
    }
    if(link2router.find(std::make_pair(link_id,cost))==link2router.end()){
        ret1 = link2router.insert ( std::pair<pair,link_set>(std::make_pair(link_id,cost),link_set()) );
        if (ret1.second==false)
            return false;
        else
            link2router[std::make_pair(link_id,cost)].push_back(router_id);
    }
    else{
        link2router[std::make_pair(link_id,cost)].push_back(router_id);
        if(link2router[std::make_pair(link_id,cost)].size()>2){return false;}
        pair router_pair = std::make_pair(link2router[std::make_pair(link_id,cost)][0],link2router[std::make_pair(link_id,cost)][1]);
        pair linkdata = std::make_pair(link_id,cost);
        
        update_adj_matrix(router_pair, linkdata, adj_matrix);
    }    
    return true;
}

int broadcast_lspdu(int socket, const struct sockaddr *dest, socklen_t dlen, struct pkt_LSPDU* pkt_lspdu_orig, struct circuit_DB* circuit, int router_id, std::map<unsigned int, unsigned int>&hello_map,FILE* logfd){
    for(int i=0;i<circuit->nbr_link;i++){        
        if(pkt_lspdu_orig->via==(unsigned int)(circuit->linkcost[i].link)){continue;}
        if(hello_map.find(pkt_lspdu_orig->via)==hello_map.end()){continue;}
        //std::cout<<"Broadcasting "<<pkt_lspdu_orig->router_id<<","<<pkt_lspdu_orig->link_id<<" to "<< (circuit->linkcost[i]).link<<std::endl;
        unsigned char buffer[5*32] = {0};
        unsigned char *ptr;
        struct pkt_LSPDU pkt_lspdu = {(unsigned int)router_id,pkt_lspdu_orig->router_id,pkt_lspdu_orig->link_id,pkt_lspdu_orig->cost,(unsigned int)((circuit->linkcost[i]).link)};
        fprintf(logfd, "R%d sends an LSPDU: sender %d router_id %u link_id %u cost %u via %u\n",router_id,pkt_lspdu.sender,pkt_lspdu.router_id, pkt_lspdu.link_id,pkt_lspdu.cost,pkt_lspdu.via);
        ptr = serialize_pkt_LSPDU(buffer, (const struct pkt_LSPDU*)&pkt_lspdu);
        if(sendto(socket, buffer, ptr - buffer, MSG_CONFIRM, dest, dlen) != ptr - buffer){
            return 1;
        }
    }
    return 0;
}
int update_lsdb(struct circuit_DB *ls_DB, struct pkt_LSPDU* pkt_lspdu,FILE* logfd){
    unsigned int router_id = pkt_lspdu->router_id;
    unsigned int nbr_link = ls_DB[router_id-1].nbr_link;
    nbr_link++;
    if(nbr_link>4){
        perror("Error: number of links exceeded 4");
        return 1;
    }
    ls_DB[router_id-1].nbr_link++;
    ls_DB[router_id-1].linkcost[nbr_link-1].link=pkt_lspdu->link_id;
    ls_DB[router_id-1].linkcost[nbr_link-1].cost=pkt_lspdu->cost;
    return 0;
}

void update_adj_matrix(pair router, pair link, pair adj_matrix[][5]){
    adj_matrix[router.first-1][router.second-1] = link;
    adj_matrix[router.second-1][router.first-1] = link;
}

void spf(pair adj_matrix[][5], int router_id, struct RIB * route_info, FILE* logfd){
    //initialization
    struct node{
        unsigned int distance;
        unsigned int hop_link;
        unsigned int hop_router;
        unsigned int done;
    };
    struct node all_node[5];
    for(int i=0;i<5;i++){
        //std::cout<<"initialization loop"<<std::endl;
        all_node[i].distance = adj_matrix[router_id-1][i].second;
        all_node[i].hop_link = adj_matrix[router_id-1][i].first;
        if(all_node[i].distance!=UINT_MAX)
        {
            all_node[i].hop_router = i+1;
        }
        else
            all_node[i].hop_router = 0;
        all_node[i].done = 0;
    }
    int done=0;
    //add source node to done;
    all_node[router_id-1].done=1;
    done++;

    while(done!=5){
        //std::cout<<"dijkstra loop: "<<done<<std::endl;
        //find router not done yet with minimum distance
        int next_router = -1;
        for(int i=0;i<5;i++){
            //std::cout<<"for loop 1: "<<i<<std::endl;
            if(all_node[i].done){continue;}
            if(next_router==-1)
                next_router = i+1;
            else{
                if(all_node[next_router-1].distance>all_node[i].distance)
                    next_router = i+1;
            }
        }
        all_node[next_router-1].done = 1;
        done++;
        //std::cout<<"dijkstra loop done update: "<<done<<std::endl;
        //update distance and hop of all neighbors of next_router:
        for(int i=0;i<5;i++){
            if(all_node[i].done){continue;}
            if(adj_matrix[next_router-1][i].first!=0 && all_node[i].distance > all_node[next_router-1].distance + adj_matrix[next_router-1][i].second)
            {
                all_node[i].distance = all_node[next_router-1].distance + adj_matrix[next_router-1][i].second;
                all_node[i].hop_link = all_node[next_router-1].hop_link;
                all_node[i].hop_router = all_node[next_router-1].hop_router;
            }
            
        }
    }
    //Updated?
    for(int i=0;i<5;i++){
        if(route_info->update)
            {
                route_info->route[i].distance = all_node[i].distance;
                route_info->route[i].hop_router = all_node[i].hop_router;   
            }
        else if(route_info->route[i].distance!=all_node[i].distance||route_info->route[i].hop_router!=all_node[i].hop_router)
            {
                route_info->route[i].distance = all_node[i].distance;
                route_info->route[i].hop_router = all_node[i].hop_router;
                route_info->update = 1;
                
            } 
    }

    //print RIB
    //if(route_info->update)
    if(true)
    {
        //std::cout<<"===========RIB============"<<std::endl;
        fprintf(logfd, "#RIB\n");
        for(int i=0;i<5;i++){
            //if(hello_map.find(all_node[i].hop_link)!=hello_map.end())
            //    std::cout<<"R"<<router_id<<" -> "<<"R"<<i+1<<" -> "<<"R"<<hello_map.find(all_node[i].hop_link)->second<<","<<all_node[i].distance<<std::endl;
            if(i+1==router_id)
                fprintf(logfd, "R%d -> R%d -> Local, 0\n",router_id,i+1);
                //std::cout<<"R"<<router_id<<" -> "<<"R"<<i+1<<" -> Local,0"<<std::endl;
            else if(all_node[i].hop_router!=0)
                fprintf(logfd, "R%d -> R%d -> R%u, %u\n",router_id,i+1,route_info->route[i].hop_router,route_info->route[i].distance);
                //std::cout<<"R"<<router_id<<" -> "<<"R"<<i+1<<" -> "<<"R"<<route_info->route[i].hop_router<<","<<route_info->route[i].distance<<std::endl;
            else
                fprintf(logfd, "R%d -> R%d -> INF, INF\n",router_id,i+1);
                //std::cout<<"R"<<router_id<<" -> "<<"R"<<i+1<<" -> INF,INF"<<std::endl;
        }
        route_info->update = 0;
    }
    
}

void printCircuitDB(struct circuit_DB* circuit, int router_id,FILE* logfd){
    fprintf(logfd, "R%d receives a CIRCUIT_DB\n", router_id);
    fprintf(logfd, "# Topology database\n");
    fprintf(logfd, "R%d -> R%d -> nbr_link %u\n",router_id , router_id, circuit->nbr_link);
    for(int i=0;i<circuit->nbr_link;i++){
        fprintf(logfd, "R%d -> R%d -> link %u cost %u\n",router_id, router_id, circuit->linkcost[i].link,circuit->linkcost[i].cost);    
    }
}

void printLSDB(struct circuit_DB * lsdb, int router_id,FILE* logfd){
    fprintf(logfd, "# Topology database\n");
    for(int i=0;i<NBR_ROUTER;i++){
        if(lsdb[i].nbr_link>0)
            fprintf(logfd,"R%d -> R%d nbr_link %u\n",router_id, i+1, lsdb[i].nbr_link);
        for(int j=0;j<lsdb[i].nbr_link;j++){
            fprintf(logfd, "R%d -> R%d link %u cost %u\n",router_id,i+1,lsdb[i].linkcost[j].link,lsdb[i].linkcost[j].cost);    
        }
    }
}

int isLittleEndean()  
{ 
	unsigned int i = 1; 
	char *c = (char*)&i; 
	if (*c)
		return 1; //little endian
	else
		return 0; //big endian
} 
