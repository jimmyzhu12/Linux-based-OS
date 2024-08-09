#ifndef _RTL8139_H
#define _RTL8139_H
// Reference from https://wiki.osdev.org/RTL8139 and from https://www.cs.usfca.edu/~cruse/cs326f04/RTL8139D_DataSheet.pdf Registers Section

// https://wiki.qemu.org/Documentation/Networking
// To use the rtl8139, the step zero is to add -netdev user,id=n0 -device rtl8139,netdev=n0 in "Target" of qumu machine

#include "../lib.h"
#include "../pci.h"
#include "../i8259.h"
#include "../Network/ethernet.h"
#include "../page.h"

// This ID is find by running pci_brute_force_enumerate()
#define BUS_ID_RTL8139      0
#define DEVICE_ID_RTL8139   3
#define MAX_BUFFER_SIZE_RTL8139 (8192+16+1500)    // recommended by https://wiki.osdev.org/RTL8139, 8192+16+1500 = 970
#define BUFFER_SIZE_RTL8139     8192

#define PHYS_RTLSEND    0x20000000      // 512MB

#define TOK 4   // Transmit
#define ROK 1   // Receive
#define MASK_FOR_RX_POINTER (~3)
#define OFFSET_CAPR 0x38    // Current Address of Packet Read

uint8_t mac_address[6]; // the MAC address of the device rtl_8139
uint32_t ioaddr;
uint32_t irq_rtl8139;
uint8_t rx_buffer[MAX_BUFFER_SIZE_RTL8139];
uint32_t receive_ptr_in_buffer;
uint32_t reg_to_use;


void turn_on_rtl8139();
void rtl8139_init();
void rtl8139_send(uint32_t content_address, uint32_t length);
void rtl8139_receive();
void print_self_mac_address();
void rtl8139_handler();

// paging for network, actually not used
pte_t pte_net[NUM_PTE] __attribute__ ((__aligned__ (SIZE_4KB)));

void rtl8139_page_init();


#endif


