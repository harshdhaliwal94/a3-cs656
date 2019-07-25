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

//unsigned char * deserialize_unsigned_int(unsigned char *buffer, int *value);

void deserialize_circuit_DB(char * buffer, struct circuit_DB* circuit, int size, int num_bytes){
	if(num_bytes!=size){
		perror("Error: did not recieve correct number of bytes for circuit_DB");
		exit(EXIT_FAILURE);
    }
    memcpy ( circuit, buffer, num_bytes );
}

int send_pkt_INIT(int socket, const struct sockaddr *dest, socklen_t dlen, const struct pkt_INIT *init_pkt)
{
  unsigned char buffer[32] = {0}; 
  unsigned char *ptr;
  ptr = serialize_pkt_INIT(buffer, init_pkt);
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

void printCircuitDB(struct circuit_DB* circuit, int router_id){
    printf("===================Circuit DB of router R%d===================\n",router_id);
    printf("nbr_link = %u\n",circuit->nbr_link);
    printf("link = %u linkcost = %u\n",circuit->linkcost[0].link,circuit->linkcost[0].cost);
    printf("link = %u linkcost = %u\n",circuit->linkcost[1].link,circuit->linkcost[1].cost);
    printf("link = %u linkcost = %u\n",circuit->linkcost[2].link,circuit->linkcost[2].cost);
    printf("link = %u linkcost = %u\n",circuit->linkcost[3].link,circuit->linkcost[3].cost);
    printf("link = %u linkcost = %u\n",circuit->linkcost[4].link,circuit->linkcost[4].cost);
    printf("===============================================================\n"); 
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
