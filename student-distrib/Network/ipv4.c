#include "ipv4.h"

void ipv4_init()
{
    // IP address 10.0.2.15 is registered by the Internet Assigned Numbers Authority (IANA) as a part of private network 10.0.2.0/24.
    // IP addresses in the private space are not assigned to any specific organization, including your ISP (Internet Service Provider), and everyone
    // is allowed to use these IP addresses without the consent of a regional Internet registry as described in RFC 1918, unlike public IP addresses.

    // https://stackoverflow.com/questions/9808560/why-do-we-use-10-0-2-2-to-connect-to-local-web-server-instead-of-using-computer
    // 10.0.2.1    Router/gateway address
    // 10.0.2.2    Special alias to your host loopback interface (i.e., 127.0.0.1 on your development machine)
    // 10.0.2.3    First DNS server
    // 10.0.2.4 / 10.0.2.5 / 10.0.2.6  Optional second, third and fourth DNS server (if any)
    // 10.0.2.15   The emulated device's own network/ethernet interface
    // 127.0.0.1   The emulated device's own loopback interface

    // but actually since we're using QEMU, the network should be:
    // 10.0.2.15    Guest Operating System's Virtual Network Device
    // 10.0.2.2     Gateway
    // 10.0.2.3     DNS
    // 10.0.2.4     SMB(optional)
    // https://wiki.qemu.org/Documentation/Networking
    ipv4_address[0] = 10;
    ipv4_address[1] = 0;
    ipv4_address[2] = 2;
    ipv4_address[3] = 15;
}


void ipv4_send(uint8_t* dest, uint8_t protocol, uint8_t* data, uint32_t length)
{
    ipv4_t ip_content;
    ip_content.version = IP_VERSION_DEFAULT;
    ip_content.IHL = IP_IHL_DEFAULT;
    ip_content.type_of_service = IP_TYPE_OF_SERVICE_DEFAULT;
    ip_content.total_length = (uint16_t) (ip_content.IHL * 4 + length);
    ip_content.identification = IP_IDENTIFICATION_DEFAULT;
    ip_content.flags = IP_FLAGS_DEFAULT;
    ip_content.fragment_offset_upper5 = 0;
    ip_content.fragment_offset_lower8 = 0;
    ip_content.time_to_live = IP_TTL_DEFAULT;
    ip_content.protocol = protocol;
    // ip checksum will be calculated later
    ip_content.header_checksum = 0;
    memcpy(ip_content.src_ip_address, ipv4_address, IPV4_LENGTH);
    memcpy(ip_content.dest_ip_address, dest, IPV4_LENGTH);
    // no options and padding
    uint8_t* data_addr = (uint8_t*) (((uint32_t*) (&ip_content)) + ip_content.IHL);

    //////// big endian problem /////////
    // total length
    ip_content.total_length = htons((uint16_t) (ip_content.IHL * 4 + length));

    // version and IHL
    uint8_t* ver_ihl_ptr = (uint8_t*) &(ip_content);
    *ver_ihl_ptr = adjust_endian_in_one_byte(*ver_ihl_ptr, 4);

    // flags and fragment offset upper five
    uint8_t* flags_upperfive_ptr = (uint8_t*) &(ip_content) + 6;
    *flags_upperfive_ptr = adjust_endian_in_one_byte(*flags_upperfive_ptr, 3);

    // calculate and set the checksum field
    set_checksum_of_ip_content(&ip_content);
    ip_content.header_checksum = htons(ip_content.header_checksum);
    // ip_content.header_checksum = 34905;

    // set data to send
    memcpy(data_addr, data, length);

    uint8_t ip_to_send[4];
    
    if ((dest[0] == 10) & (dest[1] == 0))  // if in the QEMU subnet
    {
        ip_to_send[0] = dest[0];
        ip_to_send[1] = dest[1];
        ip_to_send[2] = dest[2];
        ip_to_send[3] = dest[3];
    }
    else    // if not in the subnet, send to gateway ip address
    {
        ip_to_send[0] = 10;
        ip_to_send[1] = 0;
        ip_to_send[2] = 2;
        ip_to_send[3] = 2;
    }
    
    // find the corresponding MAC address and send through ARP
    p_to_h_map map;
    uint32_t counter = 0;
    uint32_t looptime = 10;
    while(looptime > 0)
    {
        map = arp_lookup(ip_to_send);
        if (0 == check_mac_addr_invalidity(map.haddr))
        {
            printf_original("Address Resolution found in ipv4_send!\n");
            break;
        }
        if (counter == 0)
        {
            printf_original("Requesting mac address in ipv4_send\n");
            arp_broadcast_request_mac_addr(ip_to_send);
            // printf_original("Request ended.\n");
            looptime -= 1;
            
        }
        counter = (counter + 1) % 10;
    }

    // send!
    ethernet_send(map.haddr, ETHERTYPE_IPv4, (uint8_t*) &ip_content, (uint32_t) (SIZEOF_IPv4_T + length));
    // printf_original("IP finished.\n");

}



void set_checksum_of_ip_content(ipv4_t* ip_ptr)
{
    // according to https://www.freesoft.org/CIE/RFC/791/12.htm
    // "For purposes of computing the checksum, the value of the checksum field is zero"
    ip_ptr->header_checksum = 0;

    // "The checksum field is the 16 bit one's complement of the one's complement sum of all 16 bit words in the header"

    // 10 16-bit numbers to calculate in total
    uint16_t* calculate_ptr = (uint16_t*) ip_ptr;
    int i = 0;
    uint32_t checksum = 0;
    for (i = 0; i < 10; i++)
    {
        checksum += ntohs(*calculate_ptr);
        calculate_ptr += 1;
    }

    // move the carry out bits to the end
    while(checksum > 0xFFFF)
    {
        checksum = (checksum >> 16) + (checksum & 0xFFFF);
    }

    uint16_t return_value = (uint16_t) checksum;
    ip_ptr->header_checksum = ~return_value;
}


void ipv4_receive(uint8_t* content)
{
    ipv4_t* ipv4_packet = (ipv4_t*) content;
    uint8_t version_IHL = adjust_endian_in_one_byte((*(uint8_t*) ipv4_packet), 4);
    version_IHL_t* version_IHL_ptr = (version_IHL_t*) &version_IHL;
    uint8_t type_of_service = ipv4_packet->type_of_service;
    //uint16_t total_length = ntohs(ipv4_packet->total_length);
    uint8_t* ipv4_data = (uint8_t*) content + 4 * version_IHL_ptr->IHL; 
    // uint8_t* ipv4_data = (uint8_t*) content + SIZEOF_IPv4_T; 
    if(type_of_service == IP_UDP_NUMBER)
    {
        udp_receive(ipv4_data);
    }
    else
    {
        // not handled
    }


}

