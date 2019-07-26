 #include "util.h"

// Driver code 
int main(int argc, char** argv) {
	if(argc!=5){
		printf("Error in arguments\n");
		exit(1);
	}
	int router_id = atoi(argv[1]);
	char* nse_host = argv[2];
	int nse_port = atoi(argv[3]);
	int router_port = atoi(argv[4]);
    int sockfd; 
    char buffer[MAXLINE] = {0};
    int opt =1; 
    struct sockaddr_in router_addr,nse_addr;
    //cicuit Database:
    struct circuit_DB circuit;
    // Link state Database
    struct circuit_DB ls_DB[5];
    memset(&ls_DB[0],0,5*sizeof(ls_DB[0]));
  
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 

    //Set socket options for port re-use
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        perror("setsocketopt");
        exit(EXIT_FAILURE);
    }
  
    memset(&router_addr, 0, sizeof(router_addr));
    memset(&router_addr, 0, sizeof(nse_addr));  

    // Filling nse information 
    nse_addr.sin_family = AF_INET; 
    nse_addr.sin_port = htons((unsigned short)nse_port); 

    char nse_ip[100]={'\0'};
    if(hostname_to_ip(nse_host , nse_ip)){
      perror("Error: hostname to ip");
      exit(EXIT_FAILURE);
    }
    //Converting hostname to ip
    if(inet_pton(AF_INET, nse_ip, &nse_addr.sin_addr)<=0){
        perror("Invalid nse hostname");
        exit(EXIT_FAILURE);
    }

    // Filling router information
    router_addr.sin_family = AF_INET; 
    router_addr.sin_port = htons((unsigned short)router_port); 
    router_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the router socket 
    if ( bind(sockfd, (const struct sockaddr *)&router_addr, sizeof(router_addr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }

    //Send INIT packets to nse
    int n, len;
    struct pkt_INIT init_pkt = {(unsigned int)router_id};
    memset(&circuit, 0, sizeof(struct circuit_DB));

    if(send_pkt_INIT(sockfd, (const struct sockaddr *) &nse_addr,sizeof(nse_addr), (const struct pkt_INIT*)&init_pkt)){
            perror("Error: failed to send INIT packet");
            exit(EXIT_FAILURE);
    }
    //Recieve circuit_DB
    n = recvfrom(sockfd, buffer, sizeof(circuit), 0, NULL, NULL);
    deserialize_circuit_DB(buffer, &circuit, sizeof(struct circuit_DB), n);
    //Updating lsdb
    ls_DB[router_id-1] = circuit;
    //Printing the recieved circuit_DB
    printCircuitDB(&circuit, router_id);  
    //Send Hello to neighbors
    if(send_hello_all(sockfd, (const struct sockaddr *) &nse_addr,sizeof(nse_addr), (struct circuit_DB *)&circuit, router_id)){
            perror("Error: failed to send INIT packet");
            exit(EXIT_FAILURE);
    }
    //Infinite loop
    while(true)
    {
    	//recieve hello or lspdu
    	n = recvfrom(sockfd, buffer, MAXLINE, 0, NULL, NULL);
    	if (n == sizeof(struct pkt_HELLO))
    	{
    		struct pkt_HELLO pkt_hello;
    		memcpy ( &pkt_hello, buffer, n );
    		std::cout<<"Recieved Hello from router "<<pkt_hello.router_id<<" via link "<<pkt_hello.link_id<<std::endl;
    		//Send all lspdu from ls db
    		if(reply_hello(sockfd, (const struct sockaddr *) &nse_addr,sizeof(nse_addr), (struct circuit_DB *)&ls_DB[0], router_id, (struct pkt_HELLO*)&pkt_hello)){
    			perror("Error: Couldn't reply to neighbour with lspdu");
    			exit(EXIT_FAILURE);
    		}    		
    	}
    	else if(n == sizeof(struct pkt_LSPDU)){
    		//extract the packet in structure
    		struct pkt_LSPDU pkt_lspdu;
    		memcpy ( &pkt_lspdu, buffer, n );
    		std::cout<<"Recieved LSPDU: "<<pkt_lspdu.sender<<","<<pkt_lspdu.via<<","<<pkt_lspdu.router_id<<","<<pkt_lspdu.link_id<<","<<pkt_lspdu.cost<<std::endl;
    		//TODO: Check if lsdb already exists i.e. unique(router_id,link_id)
    		ls_DB[pkt_lspdu.router_id-1].nbr_link++;
    		ls_DB[pkt_lspdu.router_id-1].linkcost[ls_DB[pkt_lspdu.router_id-1].nbr_link-1].link=pkt_lspdu.link_id;
    		ls_DB[pkt_lspdu.router_id-1].linkcost[ls_DB[pkt_lspdu.router_id-1].nbr_link-1].cost=pkt_lspdu.cost;
    	}
    	else{
    		perror("Error: Recieved Invalid Hello packet");
            exit(EXIT_FAILURE);
    	}

    }
    //SHOULD NEVER REACH HERE
    //Close the socket and exit
    close(sockfd); 
    return 0; 
} 