#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include "acn.h"
#include "PiPixelController.h"

#define ETH_BUFFER_SIZE 1500
#define ACNSDT 5568

int acn_init(uint16_t port)
{
    struct sockaddr_in servaddr, cliaddr; 
    int sockfd;

// Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
      
    // Filling server information 
    servaddr.sin_family    = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(port); 
      
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
            sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } else
    {
        return sockfd;
    }
}
void *acn_listen(void *listen_parameters)
{
    int len,n;
    int sockfd;
    struct sockaddr_in cliaddr;
    uint8_t eth_buffer[ETH_BUFFER_SIZE];
    ACNPacket *acn_packet;
    int universe;
    uint8_t sequence;
    int buffer_offset;
    acnParam *parameters = listen_parameters;

    memset(&cliaddr, 0, sizeof(cliaddr)); 
    len = sizeof(cliaddr);

    sockfd = acn_init(ACNSDT);
    while(sockfd)
    {
        n = recvfrom(sockfd, (char *)eth_buffer, ETH_BUFFER_SIZE,  
                MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
                &len); 
        acn_packet = (ACNPacket*)eth_buffer;
//        printf("Received: %s\n",acn_packet->FL.SourceName);
        if(!strcmp(acn_packet->RL.ACNPacketIdentifier,"ASC-E1.17"))
        {
            universe = getUniverse(acn_packet);
            sequence = getSequence(acn_packet);
//            printf("Universe : %i Sequence: %i\n", universe,sequence); 
            if((universe >= parameters->universeStart)&&(universe <= parameters->universeEnd))
            {
                buffer_offset = (universe - parameters->universeStart) * 512;
                memcpy(parameters->buffer + buffer_offset,acn_packet->DMP.Properties.Data,getPropertyCount(acn_packet));   
            }
        }
    }
}

// Get the universe number and convert to host integer
int getUniverse(ACNPacket *Packet)
{
	return (Packet->FL.UniverseH<<8)+Packet->FL.UniverseL;
}

// Get the number of data bytes and convert to host integer
int getPropertyCount(ACNPacket *Packet)
{
	return(Packet->DMP.Properties.PropertyCountH<<8)+Packet->DMP.Properties.PropertyCountL;
}

uint8_t getSequence(ACNPacket *Packet)
{
    return(Packet->FL.Sequence);
}