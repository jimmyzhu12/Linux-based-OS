#ifndef IDT_LINKAGE_H
#define IDT_LINKAGE_H

extern void irq_Timer_Chip();
extern void irq_Keyboard();
extern void irq_Secondary();
extern void irq_Serial_Port();
extern void irq_RTC();
extern void irq_ETH0();
extern void irq_PS2();
extern void irq_IDE0();

// for Page Fault exception
extern void excp_PF_modified();
extern void sys_Linkage();

// cp5
extern void scheduler_switch(unsigned int a, unsigned int b);

#endif
