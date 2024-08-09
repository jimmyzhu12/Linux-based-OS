// Reference: https://www.winsocketdotnetworkprogramming.com/winsock2programming/winsock2advancediphelperfunction13c.html
//            https://wiki.osdev.org/ARP

#include "arp.h"

void arp_init(){
    num_maps = 0;
    // initialize the mac address for broadcast
    int i;
    for (i = 0; i < HARDWARE_ADDRESS_LENGTH; i++)
    {
        mac_addr_for_broadcast[i] = 0xFF;
    }
}


void arp_broadcast_request_mac_addr(uint8_t* dest_ip)
{
    arp_t arp_content;
    arp_content.htype = htons(ARP_TYPE_FOR_ETHERNET);
    arp_content.ptype = htons(ARP_TYPE_FOR_IPv4);
    arp_content.hlen = HARDWARE_ADDRESS_LENGTH;
    arp_content.plen = PROTOCOL_ADDRESS_LENGTH;
    arp_content.opcode = htons(ARP_REQUEST);  
    // load mac address and ip address
    copy_mac_addr(arp_content.srchw, mac_address);
    copy_ip_addr(arp_content.srcpr, ipv4_address);
    copy_mac_addr(arp_content.dsthw, mac_addr_for_broadcast);
    copy_ip_addr(arp_content.dstpr, dest_ip);

    // send!
    ethernet_send(mac_addr_for_broadcast, ETHERTYPE_ARP, (uint8_t*) &arp_content, sizeof(arp_t));
}


void arp_receive(arp_t* arp_content)
{
    // printf_original("Pass to arp protocol!\n");
    if ((ntohs(arp_content->htype) != ARP_TYPE_FOR_ETHERNET) | (ntohs(arp_content->ptype) != ARP_TYPE_FOR_IPv4))
    {
        // not handled. just ignore
        return;
    }

    // two possibilities: received broadcast request reply, or received broadcast request
    if (ntohs(arp_content->opcode) == ARP_REPLY)
    {
        // printf_original("An arp reply!\n");
        // record the ip-mac pair
        arp_insert_map(arp_content->srcpr, arp_content->srchw);
    }
    else if (ntohs(arp_content->opcode) == ARP_REQUEST)
    {
        // see if being requested
        if (0 == ip_addr_cmp(arp_content->dstpr, ipv4_address))
        {
            // being requested. reply
            arp_t reply;
            reply.htype = htons(ARP_TYPE_FOR_ETHERNET);
            reply.ptype = htons(ARP_TYPE_FOR_IPv4);
            reply.hlen = HARDWARE_ADDRESS_LENGTH;
            reply.plen = PROTOCOL_ADDRESS_LENGTH;
            reply.opcode = htons(ARP_REPLY);  
            // load mac address and ip address
            copy_mac_addr(reply.srchw, mac_address);
            copy_ip_addr(reply.srcpr, ipv4_address);
            copy_mac_addr(reply.dsthw, arp_content->srchw);
            copy_ip_addr(reply.dstpr, arp_content->srcpr);

            // send!
            ethernet_send(arp_content->srchw, ETHERTYPE_ARP, (uint8_t*) &reply, sizeof(arp_t));
        }
    }
    else
    {
        // not handled.
    }
}


p_to_h_map arp_lookup(uint8_t* paddr)
{
    // printf_original("Looking up ARP\n");
    int i;  // iterator
    p_to_h_map result;    // the lookup result
    copy_ip_addr(result.paddr, paddr);
    // clear result.haddr
    int j;
    for (j = 0; j < HARDWARE_ADDRESS_LENGTH; j++)
    {
        result.haddr[j] = 0;
    }
    
    
    for (i = 0; i < num_maps; i++)
    {
        if (0 == ip_addr_cmp(arp_table[i].paddr, paddr))
        {
            copy_mac_addr(result.haddr, arp_table[i].haddr);
            return result;
        }
    }
    // if not found, the returned map will have haddr to be all 0
    return result;
}


void arp_insert_map(uint8_t* paddr, uint8_t* haddr)
{
    p_to_h_map result = arp_lookup(paddr);
    if(0 == check_mac_addr_invalidity(result.haddr))    // already in the table
    {
        // find and change the map
        // printf_original("Changing!\n");
        int i = 0;
        for (i = 0; i < num_maps; i++)
        {
            if (0 == ip_addr_cmp(arp_table[i].paddr, paddr))
            {
                copy_mac_addr(arp_table[i].haddr, haddr);
            }
        }
    }
    else    // insert a new one
    {
        // printf_original("Inserting!\n");
        copy_ip_addr(arp_table[num_maps].paddr, paddr);
        // printf_original("...\n");
        copy_mac_addr(arp_table[num_maps].haddr, haddr);
        num_maps += 1;
        // printf_original("Inserted.\n");
    }
}



void copy_mac_addr(uint8_t* mac1, uint8_t* mac2)
{
    int i;
    for (i = 0; i < HARDWARE_ADDRESS_LENGTH; i++)
    {
        mac1[i] = mac2[i];
    }
}


void copy_ip_addr(uint8_t* ip1, uint8_t* ip2)
{
    int i;
    for (i = 0; i < PROTOCOL_ADDRESS_LENGTH; i++)
    {
        ip1[i] = ip2[i];
    }
}


// 0 for valid, else invalid
uint32_t check_mac_addr_invalidity(uint8_t* haddr)
{
    uint32_t zero_count = 0;
    int i;
    for (i = 0; i < HARDWARE_ADDRESS_LENGTH; i++)
    {
        if (haddr[i] == 0)
        {
            zero_count += 1;
        }
    }
    if (zero_count == 6)    // invalid
    {
        return 1;
    }
    else
    {
        return 0;
    }
}



