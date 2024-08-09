#ifndef _IPv4_H
#define _IPv4_H
// Reference from http://freesoft.org/CIE/RFC/791/index.htm
#include "../lib.h"
#include "../Device/rtl8139.h"
#include "ethernet.h"
#include "arp.h"
#include "udp.h"
#define IPV4_LENGTH  4

// IP Protocol Numbers referenced from https://en.wikipedia.org/wiki/List_of_IP_protocol_numbers#:~:text=List%20of%20IP%20protocol%20numbers%20%20%20,%20RFC%20823%20%2041%20more%20rows%20
#define IP_TCP_NUMBER   0x06
#define IP_UDP_NUMBER    0x11

// default values for ip protocol
#define IP_VERSION_DEFAULT  4  
#define IP_IHL_DEFAULT      5  // five 32-bit "words"
#define IP_TYPE_OF_SERVICE_DEFAULT  0
#define IP_IDENTIFICATION_DEFAULT   0
// not sure!
#define IP_FLAGS_DEFAULT    0x02    // don't fragment, which means cannot exceed MTU, which is enough for simple communication    
#define IP_FRAGMENT_OFFSET_UPPER_DEFAULT    0
#define IP_FRAGMENT_OFFSET_LOWER_DEFAULT    0
#define IP_TTL_DEFAULT     64  // recommended by RFC1700
#define SIZEOF_IPv4_T   20
#define MAGIC_LARGE_IPv4    100

// Internet Hearder Format
typedef struct ipv4_t{
    uint8_t version    : 4;
    uint8_t IHL        : 4;
    uint8_t type_of_service;
    uint16_t total_length;
    uint16_t identification;
    uint8_t flags      : 3;
    uint8_t fragment_offset_upper5  : 5;
    uint8_t fragment_offset_lower8;
    uint8_t time_to_live;
    uint8_t protocol;
    uint16_t header_checksum;
    uint8_t src_ip_address[IPV4_LENGTH];
    uint8_t dest_ip_address[IPV4_LENGTH];
    uint8_t* data[MAGIC_LARGE_IPv4];
} ipv4_t;


typedef struct version_IHL_t{
    uint8_t version :4;
    uint8_t IHL     :4;
} version_IHL_t;


uint8_t ipv4_address[IPV4_LENGTH];

void ipv4_init();
void ipv4_send(uint8_t* dest, uint8_t protocol, uint8_t* data, uint32_t length);
void ipv4_receive(uint8_t* content); 
void set_checksum_of_ip_content(ipv4_t* ip_ptr);


#endif
