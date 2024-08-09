/* Copyright (c) 2022 by Hanwen Liu. */

/* 
 * vga.h
 * 
 * tab:8
 * 
 * Author:        Hanwen Liu
 * Version:       1
 * Creation Date: Wed Apr 27  19:52:07  2022     
 * Filename:      vga.h
 * History:
 *    HL    1     Wed Apr 27  19:52:07  2022    
 *        Adaptaed from vga.h of SVGAlib 1.4.3. Idea is triggered by Zikai Liu.
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

/* Extended for svgalib by Harm Hanemaayer and Hartmut Schirmer */

#ifndef VGA_H
#define VGA_H

/* remember to check WARNNING */

#include "../macros.h"
#include "libvga.h"
#include "../types.h"

extern int vga_init();
extern int vga_setmode(int mode);
extern void vga_setpage(unsigned int p);

extern int vga_screenon(void);
extern int vga_screenoff(void);
extern int vga_clear(void);

extern info_t* cur_vga_info;

/* variables used to shift between monchrome and color emulation */
extern int __svgalib_CRT_I;			/* current CRT index register address */
extern int __svgalib_CRT_D;			/* current CRT data register address */
extern int __svgalib_IS1_R;			/* current input status register address */

extern unsigned char* BANKED_MEM_POINTER, * LINEAR_MEM_POINTER, * MMIO_POINTER;
extern unsigned char* B8000_MEM_POINTER;
extern unsigned long int __svgalib_banked_mem_base, __svgalib_banked_mem_size;
extern unsigned long int __svgalib_mmio_base, __svgalib_mmio_size;
extern unsigned long int __svgalib_linear_mem_base, __svgalib_linear_mem_size;

/* Extensions to VGAlib v1.2: */

/* blit flags */
#define HAVE_BITBLIT 1
#define HAVE_FILLBLIT 2
#define HAVE_IMAGEBLIT 4
#define HAVE_HLINELISTBLIT 8
#define HAVE_BLITWAIT 16

/* other flags */
#define HAVE_RWPAGE 1		/* vga_setreadpage() / vga_setwritepage() available */
#define IS_INTERLACED 2		/* mode is interlaced */
#define IS_MODEX 4		/* ModeX style 256 colors */
#define IS_DYNAMICMODE 8	/* Dynamic defined mode */
#define CAPABLE_LINEAR 16	/* Can go to linear addressing mode. */
#define IS_LINEAR 32		/* Linear addressing enabled. */
#define EXT_INFO_AVAILABLE 64	/* Returned modeinfo contains valid extended fields */
#define RGB_MISORDERED 128	/* Mach32 32bpp uses 0BGR instead of BGR0. */
    /* As of this version 1.25 also used to signal if real RGB
       (red first in memory) is used instead of BGR (Mach32 DAC 4) */
#define HAVE_EXT_SET 256	/* vga_ext_set() available */

typedef struct {
	int width;
	int height;
	int bytesperpixel;
	int colors;
	int linewidth;		/* scanline width in bytes */
	int maxlogicalwidth;	/* maximum logical scanline width */
	int startaddressrange;	/* changeable bits set */
	int maxpixels;		/* video memory / bytesperpixel */
	int haveblit;		/* mask of blit functions available */
	int flags;		/* other flags */

	/* Extended fields: */

	int chiptype;		/* Chiptype detected */
	int memory;		/* videomemory in KB */
	int linewidth_unit;	/* Use only a multiple of this as parameter for set_logicalwidth and
				   set_displaystart */
	char *linear_aperture;	/* points to mmap secondary mem aperture of card (NULL if unavailable) */
	int aperture_size;	/* size of aperture in KB if size>=videomemory. 0 if unavail */
	void (*set_aperture_page) (int page);
	/* if aperture_size<videomemory select a memory page */
	void *extensions;	/* points to copy of eeprom for mach32 */
	/* depends from actual driver/chiptype.. etc. */
} vga_modeinfo;

/* Valid values for what in vga_ext_set: */
#define VGA_EXT_AVAILABLE	0	/* supported flags */
#define VGA_EXT_SET		1	/* set flag(s) */
#define VGA_EXT_CLEAR		2	/* clear flag(s) */
#define VGA_EXT_RESET		3	/* set/clear flag(s) */
#define VGA_EXT_PAGE_OFFSET	4	/* set an offset for all subsequent vga_set*page() calls */
    /* Like: vga_ext_set(VGA_EXT_PAGE_OFFSET, 42);           */
    /* returns the previous offset value.                    */
#define VGA_EXT_FONT_SIZE	5	/* the (maximal) size of the font buffer */

/* Valid params for VGA_EXT_AVAILABLE: */
#define VGA_AVAIL_SET		0	/* vga_ext_set sub funcs */
#define VGA_AVAIL_ACCEL		1	/* vga_accel sub funcs */
#define VGA_AVAIL_FLAGS		2	/* known flags for VGA_EXT_SET */
#define VGA_AVAIL_ROP		3	/* vga_accel ROP sub funcs */
#define VGA_AVAIL_TRANSPARENCY	4	/* vga_accel TRANSPARENCY sub funcs */
#define VGA_AVAIL_ROPMODES	5	/* vga_accel ROP modes supported funcs */
#define VGA_AVAIL_TRANSMODES	6	/* vga_accel TRANSPARENCY modes supported */

/* Known flags to vga_ext_set() */
#define VGA_CLUT8		1	/* 8 bit DAC entries */

/* Acceleration interface. */

/* Accel operations. */
#define ACCEL_FILLBOX			1	/* Simple solid fill. */
#define ACCEL_SCREENCOPY		2	/* Simple screen-to-screen BLT. */
#define ACCEL_PUTIMAGE			3	/* Straight image transfer. */
#define ACCEL_DRAWLINE			4	/* General line draw. */
#define ACCEL_SETFGCOLOR		5	/* Set foreground color. */
#define ACCEL_SETBGCOLOR		6	/* Set background color. */
#define ACCEL_SETTRANSPARENCY		7	/* Set transparency mode. */
#define ACCEL_SETRASTEROP		8	/* Set raster-operation. */
#define ACCEL_PUTBITMAP			9	/* Color-expand bitmap. */
#define ACCEL_SCREENCOPYBITMAP		10	/* Color-expand from screen. */
#define ACCEL_DRAWHLINELIST		11	/* Draw horizontal spans. */
#define ACCEL_SETMODE			12	/* Set blit strategy. */
#define ACCEL_SYNC			13	/* Wait for blits to finish. */
#define ACCEL_SETOFFSET			14	/* Set screen offset */
#define ACCEL_SCREENCOPYMONO		15	/* Monochrome screen-to-screen BLT. */
#define ACCEL_POLYLINE			16	/* Draw multiple lines. */
#define ACCEL_POLYHLINE			17	/* Draw multiple horizontal spans. */
#define ACCEL_POLYFILLMODE		18	/* Set polygon mode. */

/* Corresponding bitmask. */
#define ACCELFLAG_FILLBOX		0x1	/* Simple solid fill. */
#define ACCELFLAG_SCREENCOPY		0x2	/* Simple screen-to-screen BLT. */
#define ACCELFLAG_PUTIMAGE		0x4	/* Straight image transfer. */
#define ACCELFLAG_DRAWLINE		0x8	/* General line draw. */
#define ACCELFLAG_SETFGCOLOR		0x10	/* Set foreground color. */
#define ACCELFLAG_SETBGCOLOR		0x20	/* Set background color. */
#define ACCELFLAG_SETTRANSPARENCY	0x40	/* Set transparency mode. */
#define ACCELFLAG_SETRASTEROP		0x80	/* Set raster-operation. */
#define ACCELFLAG_PUTBITMAP		0x100	/* Color-expand bitmap. */
#define ACCELFLAG_SCREENCOPYBITMAP	0x200	/* Color-exand from screen. */
#define ACCELFLAG_DRAWHLINELIST		0x400	/* Draw horizontal spans. */
#define ACCELFLAG_SETMODE		0x800	/* Set blit strategy. */
#define ACCELFLAG_SYNC			0x1000	/* Wait for blits to finish. */
#define ACCELFLAG_SETOFFSET		0x2000	/* Set screen offset */
#define ACCELFLAG_SCREENCOPYMONO	0x4000	/* Monochrome screen-to-screen BLT. */
#define ACCELFLAG_POLYLINE		0x8000	/* Draw multiple lines. */
#define ACCELFLAG_POLYHLINE		0x10000	/* Draw multiple horizontal spans. */
#define ACCELFLAG_POLYFILLMODE		0x20000	/* Set polygon mode. */

/* Mode for SetTransparency. */
#define DISABLE_TRANSPARENCY_COLOR	0
#define ENABLE_TRANSPARENCY_COLOR	1
#define DISABLE_BITMAP_TRANSPARENCY	2
#define ENABLE_BITMAP_TRANSPARENCY	3

/* Flags for SetMode (accelerator interface). */
#define BLITS_SYNC			0
#define BLITS_IN_BACKGROUND		0x1

/* Raster ops. */
#define ROP_COPY			0	/* Straight copy. */
#define ROP_OR				1	/* Source OR destination. */
#define ROP_AND				2	/* Source AND destination. */
#define ROP_XOR				3	/* Source XOR destination. */
#define ROP_INVERT			4	/* Invert destination. */

/* For the poly funcs */
#define ACCEL_START			1
#define ACCEL_END			2

/*
 * valid values for what ( | is valid to combine them )
 */
#define VGA_MOUSEEVENT	1
#define VGA_KEYEVENT	2

#endif				/* VGA_H */
