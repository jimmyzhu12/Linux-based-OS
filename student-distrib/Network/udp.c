#include "udp.h"
// Reference from http://freesoft.org/CIE/RFC/768/index.htm

void udp_send(uint8_t* dst_ip, uint16_t src_port, uint16_t dst_port, uint8_t* data, uint32_t length)
{
    udp_t udp_content;
    // complete content fields
    udp_content.src_port = htons(src_port);
    udp_content.dst_port = htons(dst_port);
    udp_content.length = htons(SIZEOF_UDP_T + length);
    udp_content.checksum = 0;   // "Checksum calculation is not mandatory in UDP", https://www.gatevidyalay.com/udp-protocol-udp-header-udp/
    memcpy(udp_content.data, data, length);
    // send!
    ipv4_send(dst_ip, IP_UDP_NUMBER, (uint8_t*) &udp_content, SIZEOF_UDP_T + length);
    // printf_original("UDP finished.\n");
}

// not finished due to time limitation
void udp_receive(uint8_t* content)
{
    udp_t* udp_packet = (udp_t*) content;
    uint16_t src_port = udp_packet->src_port;
    uint16_t dst_port = udp_packet->dst_port;
    // uint16_t length = udp_packet->length;
    // DNS protocol
    if ((src_port == DNS_PORT) | (dst_port == DNS_PORT))
    {
        // upper layer DNS protocol haven't been implemented
    }
    else if ((src_port == DHCP_SERVER_PORT) | (dst_port == DHCP_CLIENT_PORT))   // DHCP
    {
        // upper layer DHCP protocol haven't been implemented
    }
}


