/* Copyright (c) 2022 by Hanwen Liu. */

/* 
 * vgaclear.c
 * 
 * tab:4
 * 
 * Author:        Hanwen Liu
 * Version:       1
 * Creation Date: Wed Apr 27  19:52:07  2022     
 * Filename:      vgaclear.c
 * History:
 *    HL    1     Sat May 7   00:16:49  2022
 *        Final revision. Fix all warnings.
 * 
 */

#include "vga.h"
#include "libvga.h"
#include "cirrus.h"
#include "macros.h"

int vga_clear(void)
{
    /* write to all bits */
    port_out(0x08, GRA_I);
    port_out(0xFF, GRA_D);
    vga_screenoff();
	// Assume video memory is 4MB, and each page is 64KB!
    int i = 4 * 1024 / 64 - 2;
    for (; i >= 0; --i) {
        vga_setpage(i);
        vga_memset(GM, 0, 1 << 16);
    }
    vga_screenon();
    return 0;
}


/* void* vga_memset(void* s, int c, unsigned int n);
 * Inputs:    void* s = pointer to memory
 *              int c = value to set memory to
 *     unsigned int n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive bytes of pointer s to value c */
void* vga_memset(void* s, int c, unsigned int n) {
    c &= 0xFF;
    asm volatile ("                 \n\
            .memset_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memset_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memset_aligned \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memset_top     \n\
            .memset_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     stosl           \n\
            .memset_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memset_done    \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%edx       \n\
            jmp     .memset_bottom  \n\
            .memset_done:           \n\
            "
            :
            : "a"(c << 24 | c << 16 | c << 8 | c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

