// Reference from https://wiki.osdev.org/RTL8139
// The four transmit start registers are 0x20, 0x24, 0x28, 0x2C
// The four status/command registers are 0x10, 0x14, 0x18, 0x1C

#include "rtl8139.h"


#define TRANSMIT_START_BASE 0x20
#define STATUS_COMMAND_BASE 0x10

void turn_on_rtl8139()
{
    outb(0x0, ioaddr + 0x52);
}


void software_reset()
{
    outb(0x10, ioaddr + 0x37);
    while((inb(ioaddr + 0x37) & 0x10) != 0){ }
} 


void get_ioaddr()
{
    uint32_t BAR0 = pci_config_read(BUS_ID_RTL8139, DEVICE_ID_RTL8139, 0, 0x10);
    ioaddr = BAR0 & (0xFFFFFFFC);
    printf_original("RTL8139 io_addr: 0x%x\n", ioaddr);
}


void get_irq8139()
{
    uint32_t register_0xF = pci_config_read(BUS_ID_RTL8139, DEVICE_ID_RTL8139, 0, 0x3C);
    irq_rtl8139 = register_0xF & 0xFF;  // the interrupt line is the last 8 bits
    printf_original("irqnum for rtl8139: 0x%x\n", irq_rtl8139);
}


void get_mac_address()
{
    mac_address[0] = inb(ioaddr);
    mac_address[1] = inb(ioaddr + 0x01);
    mac_address[2] = inb(ioaddr + 0x02);
    mac_address[3] = inb(ioaddr + 0x03);
    mac_address[4] = inb(ioaddr + 0x04);
    mac_address[5] = inb(ioaddr + 0x05);
}


void print_self_mac_address()
{
    printf_original("Self_MAC_ADDRESS: %x:%x:%x:%x:%x:%x\n", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
}


void rtl8139_init()
{
    /* from https://wiki.osdev.org/RTL8139
    * "First, you need to enable PCI Bus Mastering for this device"
    * "To do this, you can read the Command Register from the device's PCI Configuration Space, set bit 2 (bus mastering bit) and write the modified Command Register."
    *
    * from https://wiki.osdev.org/PCI
    * Command Register: 
    *  ___15-11_____________10______________________9____________________8____________7________________6_____________________5___________________________4__________________________3______________2_______________1___________0______
    *  | Reserved | Interrupt Disable | Fast Back-to-Back Enable | SERR# Enable | Reserved | Parity Error Response | VGA Palette Snoop | Memory Werite and Ivalidate Enable | Special Cycles | Bus Master | Memory Space | I/O Space |
    * 	+----------+-------------------+--------------------------+--------------+----------+-----------------------+-------------------+------------------------------------+----------------+------------+--------------+-----------+
    */ 
    uint32_t status_command = pci_config_read(BUS_ID_RTL8139, DEVICE_ID_RTL8139, 0, 4);
    status_command |= (0x00000017);
    pci_config_write(BUS_ID_RTL8139, DEVICE_ID_RTL8139, 0, 4, status_command);

    // initialization
    get_ioaddr();
    get_irq8139();
    turn_on_rtl8139();
    software_reset();
    reg_to_use = 0;
    receive_ptr_in_buffer = 0;

    // clear receive buffer
    memset((void*) rx_buffer, 0, MAX_BUFFER_SIZE_RTL8139);


    // send the chip a memory location to use
    outl((uint32_t) rx_buffer, ioaddr + 0x30); // rx_buffer is within 4M-8M, where virtual address = physical address

    // Set IMR(interrupt Mask Register) + ISR (Interrupt Service Register)
    outw(0x0005, ioaddr + 0x3C);

    // Configuring receive buffer (RCR)
    outl(0xf | (1 << 7), ioaddr + 0x44);    // (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP

    // Enable Receive and Transmitter
    outb(0x0C, ioaddr + 0x37);  // Sets the RE and TE bits high

    // get and set the mac address of this internet device
    get_mac_address();

    // enable iterrupt
    rtl8139_page_init();

    enable_irq(irq_rtl8139);

}



// According to OSdev, The RTL8139 NIC uses a round robin style for transmitting packets. It has four transmit buffer (a.k.a. transmit start) registers, and four transmit status/command registers. 
// The transmit start registers are each 32 bits long, and are in I/O offsets 0x20, 0x24, 0x28 and 0x2C. 
// The transmit status/command registers are also each 32 bits long and are in I/O offsets 0x10, 0x14, 0x18 and 0x1C. 
// Each pair of transmit start and status registers work together (i.e. registers 0x20 and 0x10 work together, 0x24 and 0x14 work together, etc.)
void rtl8139_send(uint32_t content_address, uint32_t length)
{
    // context_address must be a physical address!
    // uint8_t* sending_ptr = (uint8_t*) PHYS_RTLSEND;
    // memcpy(sending_ptr, (uint8_t*) content_address, length);
    cli();
    // load the sending address
    outl(content_address, ioaddr + TRANSMIT_START_BASE + 0x04 * reg_to_use);
    // load the length
    outl(length, ioaddr + STATUS_COMMAND_BASE + 0x04 * reg_to_use);
    reg_to_use = (reg_to_use + 1) % 4;
    sti();
    // printf_original("RTL send finished.\n");
}


// Credit to this open sourced project: https://github.com/szhou42/osdev/blob/master/src/kernel/drivers/rtl8139.c
void rtl8139_receive()
{
    uint16_t* ptr = (uint16_t*) (rx_buffer + receive_ptr_in_buffer);
    // get the length of the packet. Why the reference skips 16 bits?
    uint16_t length = *(ptr + 1);

    // points to the packet data
    ptr = ptr + 2;

    // retrieve the data
    uint8_t content[BUFFER_SIZE_RTL8139];
    network_memcpy_uint8ptr(content, (uint8_t*) ptr, length);

    // give to the upper layer(ethernet) to handle
    ethernet_receive(content, length);

    // update the receive pointer in the buffer. 
    receive_ptr_in_buffer = (receive_ptr_in_buffer + length +  4 + 3) & (MASK_FOR_RX_POINTER);

    // handle overflow condition
    while (receive_ptr_in_buffer > BUFFER_SIZE_RTL8139)
    {
        receive_ptr_in_buffer -= BUFFER_SIZE_RTL8139;
    }

    outw(receive_ptr_in_buffer - 0x10, ioaddr + OFFSET_CAPR);
    
}

void rtl8139_handler()
{
    cli();
    // printf_original("RTL Interrupt!\n");
    uint16_t status = inw(ioaddr + 0x3E);
    if (status & TOK)   // Transmit
    {
        printf_original("RTL8139 Transmitting\n");
    }
    if (status & ROK)
    {
        printf_original("RTL8139 receiving!\n");
        rtl8139_receive();
        // printf_original("RTL receive finished.\n");
    }
    // other interrupts are not permitted and not handled

    outw(0x5, ioaddr + 0x3E);
    send_eoi(irq_rtl8139);
    // printf_original("RTL handler end.\n");
    sti();
}


// actually not used.
void rtl8139_page_init()
{
    /* initialize the rtl page-table entry */
    int i;
	for (i = 0; i < NUM_PTE; ++i) {
		pte_net[i].p	  = 1;			/* not present			*/
		pte_net[i].r_w	  = 1;			/* always r/w avaliable	*/
		pte_net[i].u_s	  = 0;
		pte_net[i].pwt	  = 0;
		pte_net[i].pcd	  = 0;
		pte_net[i].a	  = 0;
		pte_net[i].d	  = 0;
		pte_net[i].pat	  = 0;
		pte_net[i].g	  = 0;
		pte_net[i].avail  = 0;
				   /* map each page to its physical address */
		pte_net[i].p_base = i;
	}

    pde[PHYS_RTLSEND >> SHIFT_4MB]._4KB.p = 1;
    pde[PHYS_RTLSEND >> SHIFT_4MB]._4KB.r_w = 1;
    pde[PHYS_RTLSEND >> SHIFT_4MB]._4KB.ps = 0;
    pde[PHYS_RTLSEND >> SHIFT_4MB]._4KB.pt_base = (uint32_t)pte_net >> SHIFT_4KB;
}

