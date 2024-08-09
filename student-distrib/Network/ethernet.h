#ifndef _ETHERNET_H
#define _ETHERNET_H
#include "../lib.h"
#include "../Device/rtl8139.h"
#include "arp.h"
#include "ipv4.h"

// Reference from https://en.wikipedia.org/wiki/EtherType
#define ETHERTYPE_IPv4  0x0800
#define ETHERTYPE_ARP   0x0806
#define MAGIC_LARGE_ETHERNET    60
#define SIZEOF_ETHERNET_T   14


// References: Computer Networking: A Top-Down Approach Figure 6-20

typedef struct ethernet_t{
    uint8_t dest[6];
    uint8_t src[6];
    uint16_t type;
    uint8_t data[MAGIC_LARGE_ETHERNET];
} ethernet_t;

void ethernet_send(uint8_t* dest, uint16_t type, uint8_t* data, uint32_t length);
void ethernet_receive(uint8_t* content, uint16_t length);


#endif



