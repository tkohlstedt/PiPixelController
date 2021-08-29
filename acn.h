#ifndef __ACN_H__
#define __ACN_H__


#include <sys/types.h>
#include <stdint.h>

typedef struct _acnParam
{
    int universeStart;
    int universeEnd;
    char *buffer;
} acnParam;

typedef struct _RootLayer{
	uint16_t PreambleSize;
	uint16_t PostambleSize;
	char ACNPacketIdentifier[12];
	uint16_t Flags;
	uint32_t Vector;
	char CID[16];
} __attribute__((packed)) RootLayer;

typedef struct _FramingLayer{
	uint16_t Flags;
	uint32_t Vector;
	char SourceName[64];
	uint8_t Priority;
	uint16_t Reserved;
	uint8_t Sequence;
	uint8_t Options;
	uint8_t UniverseH;
	uint8_t UniverseL;
} __attribute__((packed)) FramingLayer;

typedef struct _PropertyValues{
	uint8_t PropertyCountH;
	uint8_t PropertyCountL;
	uint8_t StartCode;
	uint8_t Data[512];
} __attribute__((packed)) PropertyValues;

typedef struct _DMPLayer{
	uint16_t Flags;
	uint8_t Vector;
	uint8_t AddressType;
	uint16_t PropertyAddress;
	uint16_t AddressIncrement;
	PropertyValues Properties;
} __attribute__((packed)) DMPLayer;

typedef struct _ACNPacket{
	RootLayer RL;
	FramingLayer FL;
	DMPLayer DMP;
} __attribute__((packed)) ACNPacket;

// Open the socket to listen for ACN traffic
// Returns sockfd
//
int acn_init(uint16_t port);

// Listen for ACN data
void *acn_listen(void *parameters);

// Get the universe number and convert to host integer
int getUniverse(ACNPacket *Packet);

// Get the number of data bytes and convert to host integer
int getPropertyCount(ACNPacket *Packet);

// Get the sequence number of the packet
uint8_t getSequence(ACNPacket *Packet);

#endif