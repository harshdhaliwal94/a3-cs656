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
    struct circuit_DB circuit;
  
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
    	}
    	else{
    		perror("Error: Recieved Invalid Hello packet");
            exit(EXIT_FAILURE);
    	}

    }

    //Close the socket and exit SHOULD NEVER REACH HERE
    close(sockfd); 
    return 0; 
} 