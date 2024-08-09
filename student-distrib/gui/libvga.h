/* Copyright (c) 2022 by Hanwen Liu. */

/* 
 * vgalib.h - Macro definitions shared by library modules
 * 
 * tab:8
 * 
 * Original copyrights:
 *      "SVGAlib, Copyright 1993 Harm Hanemaayer"
 *      "VGAlib version 1.2 - (c) 1993 Tommy Frandsen"
 *      "partially copyrighted (C) 1993 by Hartmut Schirmer"
 * 
 * Author:        Hanwen Liu
 * Version:       1
 * Creation Date: Wed Apr 27  19:52:07  2022     
 * Filename:      vgalib.h
 * History:
 *    HL    1     Wed Apr 27  19:52:07  2022    
 *        Adaptaed from libvga.h of SVGAlib 1.4.3. Idea is triggered by Zikai Liu.
 *    HL    2     Sat May 7   00:16:49  2022
 *        Final revision. Fix all warnings.
 * 
 */

#ifndef _LIBVGA_H
#define _LIBVGA_H

#include "../macros.h"

extern void* vga_memset(void* s, int c, unsigned int n);

/* VGA index register ports */
#define CRT_IC  0x3D4		/* CRT Controller Index - color emulation */
#define CRT_IM  0x3B4		/* CRT Controller Index - mono emulation */
#define ATT_IW  0x3C0		/* Attribute Controller Index & Data Write Register */
#define GRA_I   0x3CE		/* Graphics Controller Index */
#define SEQ_I   0x3C4		/* Sequencer Index */
#define PEL_IW  0x3C8		/* PEL Write Index */
#define PEL_IR  0x3C7		/* PEL Read Index */

/* VGA data register ports */
#define CRT_DC  0x3D5		/* CRT Controller Data Register - color emulation */
#define CRT_DM  0x3B5		/* CRT Controller Data Register - mono emulation */
#define ATT_R   0x3C1		/* Attribute Controller Data Read Register */
#define GRA_D   0x3CF		/* Graphics Controller Data Register */
#define SEQ_D   0x3C5		/* Sequencer Data Register */
#define MIS_R   0x3CC		/* Misc Output Read Register */
#define MIS_W   0x3C2		/* Misc Output Write Register */
#define IS1_RC  0x3DA		/* Input Status Register 1 - color emulation */
#define IS1_RM  0x3BA		/* Input Status Register 1 - mono emulation */
#define PEL_D   0x3C9		/* PEL Data Register */
#define PEL_MSK 0x3C6		/* PEL mask register */

/* 8514/MACH regs we need outside of the mach32 driver.. */
#define PEL8514_D	0x2ED
#define PEL8514_IW	0x2EC
#define PEL8514_IR	0x2EB
#define PEL8514_MSK	0x2EA

/* EGA-specific registers */

#define GRA_E0	0x3CC		/* Graphics enable processor 0 */
#define GRA_E1	0x3CA		/* Graphics enable processor 1 */

/* standard VGA indexes max counts */
#define CRT_C   24		/* 24 CRT Controller Registers */
#define ATT_C   21		/* 21 Attribute Controller Registers */
#define GRA_C   9		/* 9  Graphics Controller Registers */
#define SEQ_C   5		/* 5  Sequencer Registers */
#define MIS_C   1		/* 1  Misc Output Register */

/* VGA registers saving indexes */
#define CRT     0		/* CRT Controller Registers start */
#define ATT     (CRT+CRT_C)	/* Attribute Controller Registers start */
#define GRA     (ATT+ATT_C)	/* Graphics Controller Registers start */
#define SEQ     (GRA+GRA_C)	/* Sequencer Registers */
#define MIS     (SEQ+SEQ_C)	/* General Registers */
#define EXT     (MIS+MIS_C)	/* SVGA Extended Registers */

/* Shorthands for internal variables and functions */
#define GM	((char *) GRAPH_BASE)
#define SCREENON __svgalib_screenon

extern void __svgalib_delay(void);

#define GRAPH_BASE 0xA0000
#define FONT_BASE  0xA0000
#define GRAPH_SIZE 0x10000
#define FONT_SIZE  (0x2000 * 4) /* 2.0.x kernel can use 2 512 char. fonts */

/* graphics mode information */
typedef struct info_t {
    int xdim;
    int ydim;
    int colors;
    int xbytes;
    int bytesperpixel;
} info_t;

/* Background things */

extern unsigned char *__svgalib_give_graph_red(void);
extern unsigned char *__svgalib_give_graph_green(void);
extern unsigned char *__svgalib_give_graph_blue(void);

#define zero_sa_mask(maskptr) vga_memset(maskptr, 0, sizeof(sigset_t))

#endif /* _LIBVGA_H */
