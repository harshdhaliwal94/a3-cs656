#include <stddef.h>
#include <stdint.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <netdb.h>
#include <iostream>
#include <vector>
#include <map>
#include <climits>

#define MAXLINE 1024 
#define NBR_ROUTER 5

typedef std::pair<unsigned int,unsigned int> pair;
typedef std::map<pair,unsigned int> map;
typedef std::map<pair,std::vector<unsigned int> > link_map;
typedef std::vector<unsigned int> link_set;

struct pkt_HELLO
{
        unsigned int router_id;
        unsigned int link_id;
};

struct pkt_LSPDU
{
        unsigned int sender;
        unsigned int router_id;
        unsigned int link_id;
        unsigned int cost;
        unsigned int via;
};

struct pkt_INIT
{
        unsigned int router_id; /* id of the router that send the INIT PDU */
};

struct link_cost
{
        unsigned int link; /* link id */
        unsigned int cost; /* associated cost */
};

struct circuit_DB
{
        unsigned int nbr_link; /* number of links attached to a router */
        struct link_cost linkcost[NBR_ROUTER]; /* we assume that at most NBR_ROUTER links are attached to each router */
};

struct route_entry{
        unsigned int distance;
        unsigned int hop_router;
    };

struct RIB{
        bool update;
        struct route_entry route[5];
};

unsigned char * serialize_unsigned_int(unsigned char *, int );
unsigned char * serialize_char(unsigned char *, char );
unsigned char * serialize_pkt_INIT(unsigned char *, const struct pkt_INIT *);
unsigned char * serialize_pkt_HELLO(unsigned char *, const struct pkt_HELLO *);
unsigned char * serialize_pkt_LSPDU(unsigned char *, const struct pkt_LSPDU *);
void deserialize_circuit_DB(char *, struct circuit_DB*, int, int);
int send_pkt_INIT(int , const struct sockaddr *, socklen_t , const struct pkt_INIT *);
int hostname_to_ip(char * , char*);
void printCircuitDB(struct circuit_DB*, int);
void printLSDB(struct circuit_DB *, int);
int send_hello_all(int, const struct sockaddr *, socklen_t, struct circuit_DB*, int);
int reply_hello(int, const struct sockaddr *, socklen_t, struct circuit_DB*, int, struct pkt_HELLO*);
int broadcast_lspdu(int, const struct sockaddr *,socklen_t, struct pkt_LSPDU*,struct circuit_DB*, int, std::map<unsigned int, unsigned int> &);
bool unique(unsigned int, unsigned int, unsigned int, map &, link_map &, pair adj_matrix[][5]);
int update_lsdb(struct circuit_DB *, struct pkt_LSPDU*);
void update_adj_matrix(pair, pair, pair adj_matrix[][5]);
void spf(pair adj_matrix[][5], int, struct RIB *);
int isLittleEndean();