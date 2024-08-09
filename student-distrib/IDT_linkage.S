

#define ASM     1
#include "x86_desc.h"

#define irq_link(func, idx) \
.globl func                 ;\
func:                       ;\
    pushfl                  ;\
    pushal                  ;\
    pushl $idx              ;\
    jmp common_irq          ;\



common_irq:
    call irq_handler
    popl %eax
    popal
    popfl
    iret


irq_link(irq_Timer_Chip, Idx_Timer_Chip);
irq_link(irq_Keyboard, Idx_Keyboard);
irq_link(irq_Secondary, Idx_Secondary);
irq_link(irq_Serial_Port, Idx_Serial_Port);
irq_link(irq_RTC, Idx_RTC);
irq_link(irq_ETH0, Idx_Eth0);
irq_link(irq_PS2, Idx_PS2);
irq_link(irq_IDE0, Idx_Ide0);


/* : page fault linkage */
#define pf_link(func) \
.globl func                 ;\
func:                       ;\
    call print_error_code   ;\
    movl %cr2, %eax         ;\
    pushl %eax              ;\
    call print_cr2_register ;\
    popl %eax               ;\
    pushfl                  ;\
    pushal                  ;\
    jmp pagefault           ;\


pagefault:
    call excp_PF
    popal
    popfl
    iret


pf_link(excp_PF_modified);

.global scheduler_switch
scheduler_switch:

    pushl %ebp
    movl %esp, %ebp
    
    movl 8(%ebp), %eax
    movl 12(%ebp), %ebx
    movl %eax, %ebp
    movl %ebx, %esp 

    leave
    ret

/* TODO: System call linkage */


