#ifndef _ARP_H
#define _ARP_H

#include "../lib.h"
#include "../Device/rtl8139.h"
#include "ethernet.h"
#include "ipv4.h"

// Address Resolution Protocol, reference: https://wiki.osdev.org/ARP and https://datatracker.ietf.org/doc/html/rfc826
#define HARDWARE_ADDRESS_LENGTH 6
#define PROTOCOL_ADDRESS_LENGTH 4
#define ARP_TYPE_FOR_ETHERNET   1
#define MAGIC_LARGE 1000
// "The type of the protocol address that the ARP request uses. IP is 0x0800."
#define ARP_TYPE_FOR_IPv4       0x0800

// https://www.javatpoint.com/arp-packet-format
#define ARP_REQUEST 1
#define ARP_REPLY   2

typedef struct arp_t
{
    uint16_t htype; // Hardware type
    uint16_t ptype; // Protocol type
    uint8_t  hlen; // Hardware address length (Ethernet = 6)
    uint8_t  plen; // Protocol address length (IPv4 = 4)
    uint16_t opcode; // ARP Operation Code
    uint8_t  srchw[HARDWARE_ADDRESS_LENGTH]; // Source hardware address - hlen bytes (see above)
    uint8_t  srcpr[PROTOCOL_ADDRESS_LENGTH]; // Source protocol address - plen bytes (see above). If IPv4 can just be a "u32" type.
    uint8_t  dsthw[HARDWARE_ADDRESS_LENGTH]; // Destination hardware address - hlen bytes (see above)
    uint8_t  dstpr[PROTOCOL_ADDRESS_LENGTH]; // Destination protocol address - plen bytes (see above). If IPv4 can just be a "u32" type.
} arp_t;

typedef struct p_to_h_map{
    uint8_t paddr[PROTOCOL_ADDRESS_LENGTH];
    uint8_t haddr[HARDWARE_ADDRESS_LENGTH];
    // uint32_t TTL;   // hard to deal with without time.h
} p_to_h_map;


p_to_h_map arp_table[MAGIC_LARGE];
uint32_t num_maps;

uint8_t mac_addr_for_broadcast[6];


void arp_init();
void arp_update(uint8_t* ip, uint8_t* mac);
p_to_h_map arp_lookup(uint8_t* paddr);
void arp_insert_map(uint8_t* paddr, uint8_t* haddr);
void delete_entry();    // not implemented and not used
void arp_broadcast_request_mac_addr(uint8_t* dest_ip);
void arp_receive(arp_t* arp_content); 

// helper functions
void copy_mac_addr(uint8_t* mac1, uint8_t* mac2);    // copy from mac2 to mac1
void copy_ip_addr(uint8_t* ip1, uint8_t* ip2);   // copy from ip2 to ip1
uint32_t check_mac_addr_invalidity(uint8_t* haddr);

#endif


