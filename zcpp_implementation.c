#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include "PiPixelController.h"
#include "ZCPP.h"
#include "zcpp_implementation.h"

#define ETH_BUFFER_SIZE 1500

// Initialize the unicast socket
int zcpp_init()
{
    struct sockaddr_in cliaddr; 
    int sockfd;

// Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
      
    // Filling server information 
    cliaddr.sin_family    = AF_INET; // IPv4 
    cliaddr.sin_addr.s_addr = INADDR_ANY; 
    cliaddr.sin_port = htons(ZCPP_PORT); 
      
    // Bind the socket with the client address 
    if ( bind(sockfd, (const struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } else {
        return sockfd;
    }
}

// initialize the multicast socket
int zcpp_multicast_init()
{
    struct sockaddr_in cliaddr; 
    int sockfd;
    struct ip_mreq{
               struct in_addr imr_multiaddr; /* IP multicast group address */
               struct in_addr imr_interface;   /* IP address of local interface */
           } mreq;

// Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
    memset(&mreq,0,sizeof(struct ip_mreq));
      
    // Filling server information 
    cliaddr.sin_family    = AF_INET; // IPv4 
    cliaddr.sin_addr.s_addr = INADDR_ANY; 
    cliaddr.sin_port = htons(ZCPP_PORT); 
      
    // Bind the socket with the client address 
    if ( bind(sockfd, (const struct sockaddr *)&cliaddr,  
            sizeof(cliaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } else
    {
        mreq.imr_multiaddr.s_addr = inet_addr(ZCPP_MULTICAST_ADDRESS);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(sockfd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) <0)
        {
            perror("setsockopt mreq");
            exit(EXIT_FAILURE);
        } else
        return sockfd;
    }
}


void zcpp_send_discovery_response(ZCPP_packet_t *data, struct sockaddr_in server,char *controller_name)
{
    int sockfd, n;
    struct sockaddr_in servaddr;
      
    // clear servaddr
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_addr.s_addr = server.sin_addr.s_addr;
    servaddr.sin_port = server.sin_port;
    servaddr.sin_family = AF_INET;
      
    // create datagram socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
      
    // connect to server
    if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        printf("\n Error : Connect Failed \n");
        exit(0);
    }
    // find our local ip
    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    int err = getsockname(sockfd, (struct sockaddr*) &name, &namelen);
    
    ZCPP_DiscoveryResponse *response_packet;
    response_packet = malloc(sizeof(ZCPP_DiscoveryResponse));
    memset(response_packet,0,sizeof(ZCPP_DiscoveryResponse));
    memcpy(response_packet->Header.token,ZCPP_token,sizeof(ZCPP_token));
    response_packet->Header.type = ZCPP_TYPE_DISCOVERY_RESPONSE;
    response_packet->Header.protocolVersion = ZCPP_CURRENT_PROTOCOL_VERSION;
    response_packet->minProtocolVersion = ZCPP_CURRENT_PROTOCOL_VERSION;
    response_packet->maxProtocolVersion = ZCPP_CURRENT_PROTOCOL_VERSION;
    response_packet->vendor = htons(ZCPP_VENDOR_FALCON);
    response_packet->model = htons(0x0001);
    response_packet->macAddress[0] = 0xA0;
    response_packet->macAddress[1] = 0xFF;
    response_packet->macAddress[2] = 0xFF;
    response_packet->ipv4Address = name.sin_addr.s_addr; //htonl(0xC0A8087D);
    response_packet->ipv4Mask = htonl(0xFFFFFF00);
    strcpy(response_packet->userControllerName,controller_name);
    response_packet->maxTotalChannels = htons(4800);
    response_packet->pixelPorts = 4;
    response_packet->rsPorts = 0;
    response_packet->channelsPerPixelPort = htons(3600);
    response_packet->channelsPerRSPort = htons(0);
    response_packet->flags = htons(ZCPP_DISCOVERY_FLAG_SUPPORTS_VIRTUAL_STRINGS);
    response_packet->protocolsSupported = htonl(ZCPP_DISCOVERY_PROTOCOL_WS2811);

printf("My ip %i \n\r",response_packet->ipv4Address);

    // request to send datagram
    sendto(sockfd, response_packet, sizeof(ZCPP_DiscoveryResponse), 0, (struct sockaddr*)NULL, sizeof(servaddr));
  
    // close the descriptor
    close(sockfd);

}

void zcpp_process_config(ZCPP_packet_t *data,thread_ctrl *hwconfig)
{
    printf("Recieved config packet %i\n\r",ntohs(data->Configuration.sequenceNumber));
    printf("Config count %i\n\r",data->Configuration.ports);
    for(int a =0;a<data->Configuration.ports;a++)
    {
        printf("Port : %i Channels: %i  Protocol: %i String: %i Start Channel: %i \n\r",data->Configuration.PortConfig[a].port,ntohl(data->Configuration.PortConfig[a].channels),
        data->Configuration.PortConfig[a].protocol,data->Configuration.PortConfig[a].string,
        ntohl(data->Configuration.PortConfig[a].startChannel));

        hwconfig->led_string[data->Configuration.PortConfig[a].port].channel_count[data->Configuration.PortConfig[a].string] = ntohl(data->Configuration.PortConfig[a].channels);
        hwconfig->led_string[data->Configuration.PortConfig[a].port].start_channel[data->Configuration.PortConfig[a].string] = ntohl(data->Configuration.PortConfig[a].startChannel);

    }
}
void zcpp_process_extra_data(ZCPP_packet_t *data)
{

}
void zcpp_send_config_response(ZCPP_packet_t *data)
{
    printf("recieved config request\n\r");
}
void zcpp_process_data(ZCPP_packet_t *data, char *pixelbuffer)
{
    uint32_t buffer_offset;
    uint16_t len;
//    printf("Offset : %i Len: %i\n", ntohl(data->Data.frameAddress),ntohs(data->Data.packetDataLength)); 

    buffer_offset = ntohl(data->Data.frameAddress);
    len = ntohs(data->Data.packetDataLength);
    memcpy(pixelbuffer + buffer_offset,data->Data.data,len);   

}
void zcpp_process_sync(ZCPP_packet_t *data)
{

}

// listen for multicast packets
void *zcpp_multicast_listen(void *listen_parameters)
{
    int sockdc;
    ZCPP_packet_t *multi_packet;
    struct sockaddr_in servaddr;

    int len,n;
    uint8_t eth_buffer[ETH_BUFFER_SIZE];
    zcppParam *params = listen_parameters;
    char *pixelbuffer = params->buffer;


    sockdc = zcpp_multicast_init();
    while(sockdc)
    {
        n = recvfrom(sockdc, (char *)eth_buffer, ETH_BUFFER_SIZE,  
                MSG_WAITALL, ( struct sockaddr *) &servaddr, 
                &len); 
        multi_packet = (ZCPP_packet_t *)eth_buffer;
        if (!strncmp(multi_packet->Discovery.Header.token,ZCPP_token,4))
        {
//            printf("found it %i\n\r",multi_packet->Discovery.Header.type);
            switch (multi_packet->Discovery.Header.type)
            {
                case ZCPP_TYPE_DISCOVERY:
                    zcpp_send_discovery_response(multi_packet,servaddr,(char *)((zcppParam*)params->controller_name) );
                    break;
                case ZCPP_TYPE_CONFIG:
                    zcpp_process_config(multi_packet,(thread_ctrl*)((zcppParam*)params->hwconfig));
                    break;
                case ZCPP_TYPE_EXTRA_DATA:
                    zcpp_process_extra_data(multi_packet);
                    break;
                case ZCPP_TYPE_QUERY_CONFIG:
                    zcpp_send_config_response(multi_packet);
                    break;
                case ZCPP_TYPE_DATA:
                    zcpp_process_data(multi_packet,pixelbuffer);
                    break;
                case ZCPP_TYPE_SYNC:
                    zcpp_process_sync(multi_packet);
                    break;
            }
        }
    }
}

// listen for unicast packets
void *zcpp_listen(void *listen_parameters)
{
    int len,n;
    int sockfd;
    struct sockaddr_in servaddr;
    uint8_t eth_buffer[ETH_BUFFER_SIZE];
    ZCPP_packet_t *zcpp_packet;
    int universe;
    uint8_t sequence;
    int buffer_offset;
    zcppParam *params = listen_parameters;
    char *pixelbuffer = params->buffer;

    memset(&servaddr, 0, sizeof(servaddr)); 
    len = sizeof(servaddr);

    sockfd = zcpp_init(ZCPP_PORT);
    while(sockfd)
    {
        n = recvfrom(sockfd, (char *)eth_buffer, ETH_BUFFER_SIZE,  
                MSG_WAITALL, ( struct sockaddr *) &servaddr, 
                &len); 
        zcpp_packet = (ZCPP_packet_t*)eth_buffer;
        if(!strncmp(zcpp_packet->Data.Header.token,ZCPP_token,4))
        {
//            printf("Received packet %i\n\r",zcpp_packet->Data.sequenceNumber);
            switch (zcpp_packet->Discovery.Header.type)
            {
                case ZCPP_TYPE_DISCOVERY:
                    zcpp_send_discovery_response(zcpp_packet,servaddr,(char *)((zcppParam*)params->controller_name));
                    break;
                case ZCPP_TYPE_CONFIG:
                    zcpp_process_config(zcpp_packet,(thread_ctrl*)((zcppParam*)params->hwconfig));
                    break;
                case ZCPP_TYPE_EXTRA_DATA:
                    zcpp_process_extra_data(zcpp_packet);
                    break;
                case ZCPP_TYPE_QUERY_CONFIG:
                    zcpp_send_config_response(zcpp_packet);
                    break;
                case ZCPP_TYPE_DATA:
                    zcpp_process_data(zcpp_packet,pixelbuffer);
                    break;
                case ZCPP_TYPE_SYNC:
                    zcpp_process_sync(zcpp_packet);
                    break;
            }
        }
    }
}
/*
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

*/