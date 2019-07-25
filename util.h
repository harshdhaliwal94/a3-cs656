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

#define MAXLINE 1024 
#define NBR_ROUTER 5

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

unsigned char * serialize_unsigned_int(unsigned char *, int );
unsigned char * serialize_char(unsigned char *, char );
unsigned char * serialize_pkt_INIT(unsigned char *, const struct pkt_INIT *);
void deserialize_circuit_DB(char *, struct circuit_DB*, int, int);
int send_pkt_INIT(int , const struct sockaddr *, socklen_t , const struct pkt_INIT *);
int hostname_to_ip(char * , char*);
void printCircuitDB(struct circuit_DB*, int);
int isLittleEndean();