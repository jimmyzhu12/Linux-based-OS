/* Copyright (c) 2022 by Hanwen Liu. */

/* 
 * vga.c
 * 
 * tab:8
 * 
 * Author:        Hanwen Liu
 * Version:       1
 * Creation Date: Wed Apr 27  19:52:07  2022     
 * Filename:      vga.c
 * History:
 *    HL    1     Wed Apr 27  19:52:07  2022    
 *        Adaptaed from vga.c of SVGAlib 1.4.3. Idea is triggered by Zikai Liu.
 *    HL    2     Sat May 7   00:16:49  2022
 *        Final revision. Fix all warnings.
 * 
 */


/*--------------------- Original copyrights -----------------------*/
/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright (C) 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */
/* Changes by Michael Weller. */
/* Modified by Don Secrest to include Tseng ET6000 handling */
/* Changes around the config things by 101 (Attila Lendvai) */

/* The code is a bit of a mess; also note that the drawing functions */
/* are not speed optimized (the gl functions are much faster). */

#include "vga.h"
#include "cirrus.h"
#include "macros.h"
#include "gui.h"
#include "../macros.h"

unsigned int prev_page = NULL;

/* variables used to shift between monchrome and color emulation */
int __svgalib_CRT_I = CRT_IC;			/* current CRT index register address */
int __svgalib_CRT_D = CRT_DC;			/* current CRT data register address */
int __svgalib_IS1_R = IS1_RC;			/* current input status register address */

unsigned char * BANKED_MEM_POINTER = NULL, * LINEAR_MEM_POINTER, *MMIO_POINTER;
unsigned char * B8000_MEM_POINTER = NULL;
unsigned long int __svgalib_banked_mem_base, __svgalib_banked_mem_size;
unsigned long int __svgalib_mmio_base, __svgalib_mmio_size=0;
unsigned long int __svgalib_linear_mem_base=0, __svgalib_linear_mem_size=0;

unsigned char __svgalib_novga = 0;      /* have VGA circuitry on board if !=0 */
int __svgalib_screenon = 0;             /* screen visivle if != 0 */

int vga_setmode(int mode);
int vga_screenon(void);
int vga_screenoff(void);

extern void set_gui_cur_vga_info();
extern int set_mouse_range(int x, int y);
extern void* vga_memset(void* s, int c, unsigned int n);

void get_desktop();
void gui_obj_init();

int vga_init() {
    return cirrus_test();
}

void vga_setpage(unsigned int p) {
    if (p != prev_page) {
        prev_page = p;
        port_outw((p << 10) + 0x09, GRA_I);
    }
}

int vga_setmode(int mode) {
    unsigned int IF;
    cli_and_save(IF);
    {
		int i;
        /* trun off the screen for better performance and avoid syschronization issues */
        vga_screenoff();

        port_out(port_in(MIS_R) | 0x01, MIS_W);

        if (cirrus_setmode(mode, 0) == -1) {
            vga_screenon();
            restore_flags(IF);
            return -1;
        }

        cirrus_setdisplaystart(0);
        cirrus_setlogicalwidth(cur_vga_info->xbytes);
        cirrus_setlinear(0);

		set_gui_cur_vga_info();

        /* reset the screen */
        vga_clear();

		get_desktop();

		font_obj_init(-1);

		gui_obj_init();

		set_mouse_range(cur_vga_info->xdim, cur_vga_info->ydim);

		for (i = 0; i < NUMBER_OF_TERMINAL; ++i) {
			if (gui_terminal[i].x > cur_vga_info->xdim - BORDER) {
				gui_terminal[i].x = cur_vga_info->xdim - BORDER;
			}
			if (gui_terminal[i].y > cur_vga_info->ydim - BORDER) {
				gui_terminal[i].y = cur_vga_info->ydim - BORDER;
			}
		}

        vga_screenon();
    }
    restore_flags(IF);

    return 0;
}

int vga_screenon(void)
{
    SCREENON = 1;
    if(__svgalib_novga) return -1; 

#ifndef DISABLE_VIDEO_OUTPUT
    /* enable video output */
    port_in(__svgalib_IS1_R);
    __svgalib_delay();
    port_out(0x20, ATT_IW);
#endif

    return 0;
}

int vga_screenoff(void)
{
    SCREENON = 0;
    if(__svgalib_novga) return -1; 
	
#ifndef DISABLE_VIDEO_OUTPUT
    /* Disable video output */
    port_in(__svgalib_IS1_R);
    __svgalib_delay();
    port_out(0x00, ATT_IW);
#endif

    return 0;
}

void __svgalib_delay(void)
{
    int i;
    for (i = 0; i < 10; i++);
}

