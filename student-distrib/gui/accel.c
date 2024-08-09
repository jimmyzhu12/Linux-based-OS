/* Copyright (c) 2022 by Hanwen Liu. */

/* 
 * accel.c
 * 
 * tab:4
 * 
 * Author:        Hanwen Liu
 * Version:       1
 * Creation Date: Sat May 7   00:16:49  2022 
 * Filename:      accel.c
 * History:
 *    HL    1     Sat May 7   00:16:49  2022
 *        Adaptaed from accel.h of SVGAlib 1.4.3. Final revision.
 *        Fix all warnings.
 * 
 */


#include "vga.h"
#include "timing.h"
#include "accel.h"


int __svgalib_accel_screenpitch;
int __svgalib_accel_bytesperpixel;
int __svgalib_accel_screenpitchinbytes;
int __svgalib_accel_mode;
int __svgalib_accel_bitmaptransparency;

void __svgalib_InitializeAcceleratorInterface(ModeInfo * modeinfo)
{
    __svgalib_accel_screenpitch = modeinfo->lineWidth / modeinfo->bytesPerPixel;
    __svgalib_accel_bytesperpixel = modeinfo->bytesPerPixel;
    __svgalib_accel_screenpitchinbytes = modeinfo->lineWidth;
    __svgalib_accel_mode = BLITS_IN_BACKGROUND;
    __svgalib_accel_bitmaptransparency = 0;
}
