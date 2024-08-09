/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

#define MASK_ALL 0xFF       // constant used for mask
#define IR_NUM   8
#define PIC_NUM  2
#define PIT_IRQNUM 0
#define KEYBOARD_IRQNUM 1
#define RTC_IRQNUM 8
#define RTL8139_IRQ_NUM 11
#define MOUSE_IRQ_NUM 12    // subject to change


/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask = MASK_ALL; /* IRQs 0-7  */
uint8_t slave_mask = MASK_ALL;  /* IRQs 8-15 */

/* 
 *  i8259_init
 *  DESCRIPTION: initialize i8259
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: Enable the PIC to acceept interrupt
 */
void i8259_init(void) {
    // mask all interupts
    outb(MASK_ALL, MASTER_8259_DATA);
    outb(MASK_ALL, SLAVE_8259_DATA);

    // initialization
    outb(ICW1, MASTER_8259_PORT);               //ICW1: select 8259A-1 init
    outb(ICW2_MASTER, MASTER_8259_DATA);        //ICW2: 8259A-1 IR0-7 mapped to 0x20-0x27
    outb(ICW3_MASTER, MASTER_8259_DATA);        //ICW3: 8259A-1 has a secondary on IRQ2
    outb(ICW4, MASTER_8259_DATA);               //ICW4

    outb(ICW1, SLAVE_8259_PORT);                //ICW1: select 8259A-2 init
    outb(ICW2_SLAVE, SLAVE_8259_DATA);         //ICW2: 8259A-1 IR0-7 mapped to 0x28-0x2f
    outb(ICW3_SLAVE, SLAVE_8259_DATA);         //ICW3: 8259A-2 is secondary to master's IRQ2
    outb(ICW4, SLAVE_8259_DATA);               //ICW4

    // enable the slave
    enable_irq(ICW3_SLAVE);               
}

/* 
 * enable_irq
 *  DESCRIPTION: Tell I8259 to accept the interrup specified by irq_num.
 *               Change the specified mask bit to 0
 *  INPUTS:     irq_num -- range from 0 - 15
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: Enable the PIC to acceept interrupt
 */
void enable_irq(uint32_t irq_num) {
    // sanity check
    if(irq_num >= IR_NUM * PIC_NUM){return;}

    // irq_num range from 8-15 -> slave
    if(irq_num >= IR_NUM){
        slave_mask &= ~(1 << (irq_num - IR_NUM));
        outb(slave_mask, SLAVE_8259_DATA);
    }
    // irq_num range from 0-7 -> master
    else{
        if (irq_num != 0)
        {
            master_mask &= ~(1 << irq_num);
        }
        else
        {
            master_mask &= ~(0x01);
        }
        outb(master_mask, MASTER_8259_DATA);
    }

}

/* 
 * disable_irq
 *  DESCRIPTION: Tell I8259 to disable the interrup specified by irq_num.
 *               Change the specified mask bit to 1.
 *  INPUTS: irq_num -- range from 0 - 15
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: disable the PIC to acceept interrupt
 */
void disable_irq(uint32_t irq_num) {
    // sanity check
    if(irq_num >= IR_NUM * PIC_NUM){return;}

    // irq_num range from 8-15 -> slave
    if(irq_num >= IR_NUM){
        slave_mask |= (1 << (irq_num - IR_NUM));
        outb(slave_mask, SLAVE_8259_DATA);
    }
    // irq_num range from 0-7 -> master
    else{
        master_mask |= (1 << irq_num);
        outb(master_mask, MASTER_8259_DATA);
    }
}

/* 
 * send_eoi
 *  DESCRIPTION: Send end-of-interrupt signal for the specified IRQ
 *  INPUTS: irq_num -- range from 0 - 15
 *  OUTPUTS: none
 *  RETURN VALUE: none
 */
void send_eoi(uint32_t irq_num) {
    // sanity check
    if(irq_num >= IR_NUM * PIC_NUM){return;}

    // irq_num range from 8-15 -> slave
    if (irq_num >= IR_NUM){
        outb(EOI|ICW3_SLAVE, MASTER_8259_PORT);            //send eoi to the master, IR2 
        outb(EOI|(irq_num - IR_NUM), SLAVE_8259_PORT);     //send eoi to the slave
    }
    // irq_num range from 0-7 -> master
    else{
        outb(EOI|irq_num, MASTER_8259_PORT);               //send eoi to the master
    }

}


void disable_all()
{
    disable_irq(PIT_IRQNUM);
    disable_irq(KEYBOARD_IRQNUM);
    disable_irq(RTC_IRQNUM);
    disable_irq(RTL8139_IRQ_NUM);
    disable_irq(MOUSE_IRQ_NUM);
}


void enable_all()
{
    int IF;
    cli_and_save(IF);
    enable_irq(RTC_IRQNUM);
    enable_irq(RTL8139_IRQ_NUM);
    enable_irq(MOUSE_IRQ_NUM);
    enable_irq(KEYBOARD_IRQNUM);
    enable_irq(PIT_IRQNUM);
    restore_flags(IF);
}






