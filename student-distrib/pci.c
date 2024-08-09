// Reference from https://wiki.osdev.org/PCI

#include "pci.h"

uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset){
    uint32_t address;
    uint32_t lbus = (uint32_t) bus;
    uint32_t lslot = (uint32_t) slot;
    uint32_t lfunc = (uint32_t) func;
    uint32_t tmp = 0;

    // FC is 11111100. 4 byte together is 32 bits
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

    outl(address, CONFIG_ADDRESS);
    tmp = inl(CONFIG_DATA);
    return tmp;
}

void pci_config_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t data_to_write){
    uint32_t address;
    uint32_t lbus = (uint32_t) bus;
    uint32_t lslot = (uint32_t) slot;
    uint32_t lfunc = (uint32_t) func;

    // FC is 11111100. 4 byte together is 32 bits
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
    
    // do the write
    outl(address, CONFIG_ADDRESS);
    outl(data_to_write, CONFIG_DATA);
}

void checkDevice(uint16_t bus, uint8_t device){
    // if the first function exists, then the device exists.
    uint32_t response = pci_config_read(bus, device, 0, 0);
    if (response != 0xFFFFFFFF)
    {
        uint16_t device_id = (uint16_t) (response >> 16);
        uint16_t vender_id = (uint16_t) (response & 0x0000FFFF);
        printf_original("bus: %x, device: %x has Device ID: %x, Vendor ID: %x\n", bus, device, device_id, vender_id);
    }
}


void pci_brute_force_enumerate(){
    uint16_t bus;
    uint8_t device;
    for (bus = 0; bus < 256; bus++)
    {
        for (device = 0; device < 32; device++)
        {
            checkDevice(bus, device);
        }
    }
}

