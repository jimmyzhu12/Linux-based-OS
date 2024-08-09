#include "ethernet.h"

// Reference: Computer Networking: A Top-Down Approach Figure 6-20


void ethernet_send(uint8_t* dest, uint16_t type, uint8_t* data, uint32_t length)
{
    // initialize the sending structure
    ethernet_t content;
    copy_mac_addr(content.dest, dest);
    copy_mac_addr(content.src, mac_address);
    // remember to convert from host to net (little endian -> big endian)
    content.type = htons(type);
    memcpy(content.data, data, length);
    // send through rtl8139
    rtl8139_send((uint32_t) &content, SIZEOF_ETHERNET_T + length);
    // printf_original("ethernet finished.\n");
    
}



void ethernet_receive(uint8_t* content, uint16_t length)
{
    // printf_original("Pass to Ethernet protocol!\n");
    ethernet_t* ether= (ethernet_t*) content;
    uint8_t* ether_data = (uint8_t*) content + SIZEOF_ETHERNET_T; 
    // uint16_t data_length = length - SIZEOF_ETHERNET_T;
    uint16_t type_of_packet = ntohs(ether->type);
    if (type_of_packet == ETHERTYPE_ARP)
    {
        arp_receive((arp_t*) ether_data);
    }   
    else if(type_of_packet == ETHERTYPE_IPv4)
    {
        ipv4_receive(ether_data);
    } 
    // other types are not handled

    
}


