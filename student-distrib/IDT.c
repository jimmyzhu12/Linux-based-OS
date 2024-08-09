
#include "x86_desc.h"
#include "IDT.h"
#include "IDT_linkage.h"
#include "lib.h"
#include "SystemCall.h"
// Error Informations
static char* Error_Info[20] = {
    "Division Error",
    "Reserved",
    "Nonmaskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Error",
    "Invalid Opcode",
    "Devide Non Available",
    "Double Fault",
    "Coprocessor Segment",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Segment Fault",
    "General Protection",
    "Page Fault",
    "Not Used",
    "x87 FPU Floating-Point Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Float-Point Exception"
};

// print exception error information
#define EXCP_SET(func, excp_Idx) \
void func(){                   \
    cli();                     \
    printf_original("============================Error============================\n");\
    printf_original("    Exception Num: %d     Discription: %s \n", excp_Idx,  Error_Info[excp_Idx] );\
    printf_original("============================Error============================\n");    \
    sti();                     \
    halt(1);                   \
}                              \

// for debugging
// int32_t cr2_register;
// asm volatile("movl %%cr2, %0": "=r"(cr2_register));
// printf("============cr2 value: %x==============\n", cr2_register);

// Excption handler definations
EXCP_SET(excp_Division_Error, EXCP_Divide_Error);
EXCP_SET(excp_Reserved,EXCP_RESERVED);
EXCP_SET(excp_Nonmaskable_IR,EXCP_NMI);
EXCP_SET(excp_Breakpoint,EXCP_Breakpoint);
EXCP_SET(excp_Overflow,EXCP_Overflow);
EXCP_SET(excp_Bound_Error,EXCP_BOUND_Range_Exceeded);
EXCP_SET(excp_Invalid_Opcode,EXCP_Invalid_Opcode);
EXCP_SET(excp_Devide_Not_Available,EXCP_Device_Not_Available);
EXCP_SET(excp_Double_Fault,EXCP_Double_Fault);
EXCP_SET(excp_Coprocessor_Segment,EXCP_Coprocessor_Segment_Overrun);
EXCP_SET(excp_Invalid_TSS,EXCP_Invalid_TSS);
EXCP_SET(excp_Segment_Not_Present,EXCP_Segment_Not_Present);
EXCP_SET(excp_Stack_Segfault,EXCP_Stack_Segment_Fault);
EXCP_SET(excp_General_Protection,EXCP_General_Protection);
EXCP_SET(excp_PF,EXCP_Page_Fault);
EXCP_SET(excp_Not_Used,EXCP_Not_Used);
EXCP_SET(excp_x87,EXCP_FPU_Floating_Point);
EXCP_SET(excp_Alignment_Check,EXCP_Alignment_Check);
EXCP_SET(excp_Machine_Check,EXCP_Machine_Check);
EXCP_SET(excp_Dimd_Float_Point,EXCP_SIMD_Floating_Point);

// void sys_Linkage(){
//     cli();
//     // clear();
//     printf("------------------------------SYSTEM CALL--------------------------\n");
//     // while(1);
//     sti();
// }


/* 
 * idt_init
 *   DESCRIPTION: initialize the IDT table with descriptors
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: load the IDT table to memory specified by pointer idt
 */
void idt_init(){
    int i;          //loop counter
    //Generalized initialization
    for(i=0; i<NUM_VEC; i++){
        idt[i].dpl = 0;
        idt[i].present = 0;
        idt[i].seg_selector = KERNEL_CS;
        idt[i].reserved4 = 0;
        idt[i].reserved1 = 1;
        idt[i].reserved2 = 1;
        // interrupt gates clear the IF bit in EFLAGS, while trap gates don't do anything to IF. In trap, we can accept new interrupts.
        // Therefore, we should use interrupt gates for all three (exception, interrupt, system call)
        idt[i].reserved3 = 0;
        if (i < 20){
            idt[i].reserved3 = 1;
        }
        // TODO : Trap Gate!
        // We use 32-bit mode
        idt[i].size = 1;
    }

    // initialize exception
    SET_IDT_ENTRY(idt[0], excp_Division_Error);
    SET_IDT_ENTRY(idt[1], excp_Reserved);
    SET_IDT_ENTRY(idt[2], excp_Nonmaskable_IR);
    SET_IDT_ENTRY(idt[3], excp_Breakpoint);
    SET_IDT_ENTRY(idt[4], excp_Overflow);
    SET_IDT_ENTRY(idt[5], excp_Bound_Error);
    SET_IDT_ENTRY(idt[6], excp_Invalid_Opcode);
    SET_IDT_ENTRY(idt[7], excp_Devide_Not_Available);
    SET_IDT_ENTRY(idt[8], excp_Double_Fault);
    SET_IDT_ENTRY(idt[9], excp_Coprocessor_Segment);
    SET_IDT_ENTRY(idt[10], excp_Invalid_TSS);
    SET_IDT_ENTRY(idt[11], excp_Segment_Not_Present);
    SET_IDT_ENTRY(idt[12], excp_Stack_Segfault);
    SET_IDT_ENTRY(idt[13], excp_General_Protection);
    SET_IDT_ENTRY(idt[14], excp_PF_modified);
    SET_IDT_ENTRY(idt[15], excp_Not_Used);
    SET_IDT_ENTRY(idt[16], excp_x87);
    SET_IDT_ENTRY(idt[17], excp_Alignment_Check);
    SET_IDT_ENTRY(idt[18], excp_Machine_Check);
    SET_IDT_ENTRY(idt[19], excp_Dimd_Float_Point);

    // initialize interrupt 
    SET_IDT_ENTRY(idt[32], irq_Timer_Chip);
    SET_IDT_ENTRY(idt[33], irq_Keyboard);
    SET_IDT_ENTRY(idt[34], irq_Secondary);
    SET_IDT_ENTRY(idt[36], irq_Serial_Port);
    SET_IDT_ENTRY(idt[40], irq_RTC);
    SET_IDT_ENTRY(idt[43], irq_ETH0);
    SET_IDT_ENTRY(idt[44], irq_PS2);
    SET_IDT_ENTRY(idt[46], irq_IDE0);

    // initialize system call
    SET_IDT_ENTRY(idt[0x80], sys_Linkage);
    idt[0x80].dpl = 3;

}


void print_error_code(int32_t something)
{
    printf_original("============Error Code: %x============\n", something);
}

void print_cr2_register(int32_t something)
{
    printf_original("============CR2: %x============\n", something);
}


