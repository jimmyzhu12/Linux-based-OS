#include "IDT_linkage.h"
#include "lib.h"
#include "x86_desc.h"
#include "Device/keyboard_handler.h"
#include "Device/mouse_handler.h"
#include "rtc.h"
#include "Scheduler.h"
#include "Device/rtl8139.h"


/* 
 * irq_handler
 *   DESCRIPTION: call the proper interrupt handler given a IRQ index
 *   INPUTS: idx - index of interrupt in IDT
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */

void irq_handler(idx){
    cli();
    switch(idx){
        case Idx_Keyboard:
            handle_kb_interrupt();
            // printf("INTERRUPT #0x%x: Keyboard\n", idx);
            break;

        case Idx_RTC:
            rtc_handler();
            break;

        case Idx_Timer_Chip:
            pit_handler();
            break;

		case Idx_PS2:
			handle_mo_interrupt();
			break;

        case Idx_Eth0:
            rtl8139_handler();
            break;

        default:
            printf(" INTERRUPT HANDLER NOT DEFINED");
    }
    sti();
}

