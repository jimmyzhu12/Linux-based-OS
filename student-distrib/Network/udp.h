#ifndef _UDP_H
#define _UDP_H
#include "../lib.h"
#include "ipv4.h"
// Reference from http://freesoft.org/CIE/RFC/768/index.htm
#define MAGIC_LARGE_UDP    110
#define SIZEOF_UDP_T    8
// https://docs.oracle.com/en/storage/tape-storage/sl4000/slklg/default-port-numbers.html
#define DNS_PORT    53
#define DHCP_SERVER_PORT   67
#define DHCP_CLIENT_PORT   68

typedef struct udp_t
{
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
    uint8_t  data[MAGIC_LARGE_UDP];
} udp_t;


void udp_send(uint8_t* dst_ip, uint16_t src_port, uint16_t dst_port, uint8_t* data, uint32_t length);
void udp_receive(uint8_t* content);



#endif
