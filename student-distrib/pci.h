// Referenced form https://wiki.osdev.org/PCI
#ifndef _PCI_H
#define _PCI_H
#include "lib.h"

#define CONFIG_ADDRESS  0xCF8
#define CONFIG_DATA     0xCFC

uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_config_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t data_to_write);
void checkDevice(uint16_t bus, uint8_t device);
void pci_brute_force_enumerate();


#endif

