/* Copyright (c) 2022 by Hanwen Liu. */

/* 
 * interface.c
 * 
 * tab:8
 * 
 * Author:        Hanwen Liu
 * Version:       1
 * Creation Date: Sat May 7   00:16:49  2022 
 * Filename:      interface.c
 * History:
 *    HL    1     Sat May 7   00:16:49  2022
 *        Adaptaed from interface.c of SVGAlib 1.4.3. Final revision.
 *        Fix all warnings.
 * 
 */

#include "timing.h"
#include "libvga.h"		/* for __svgalib_infotable and ramdac inlines */
#include "accel.h"


/*
 * This is a temporary function that allocates and fills in a ModeInfo
 * structure based on a svgalib mode number.
 */

ModeInfo __modeinfo;

ModeInfo * __svgalib_createModeInfoStructureForSvgalibMode(info_t* info)
{
    
    ModeInfo *modeinfo = &__modeinfo;
    /* Create the new ModeInfo structure. */
    modeinfo->width = info->xdim;
    modeinfo->height = info->ydim;
    modeinfo->bytesPerPixel = info->bytesperpixel;
    switch (info->colors) {
        case 16:
	    modeinfo->colorBits = 4;
	    break;
        case 256:
	    modeinfo->colorBits = 8;
	    break;
        case 32768:
	    modeinfo->colorBits = 15;
	    modeinfo->blueOffset = 0;
	    modeinfo->greenOffset = 5;
	    modeinfo->redOffset = 10;
	    modeinfo->blueWeight = 5;
	    modeinfo->greenWeight = 5;
	    modeinfo->redWeight = 5;
	    break;
        case 65536:
	    modeinfo->colorBits = 16;
	    modeinfo->blueOffset = 0;
	    modeinfo->greenOffset = 5;
    	    modeinfo->redOffset = 11;
    	    modeinfo->blueWeight = 5;
    	    modeinfo->greenWeight = 6;
    	    modeinfo->redWeight = 5;
    	    break;
        case 256 * 65536:
    	    modeinfo->colorBits = 24;
    	    modeinfo->blueOffset = 0;
    	    modeinfo->greenOffset = 8;
    	    modeinfo->redOffset = 16;
    	    modeinfo->blueWeight = 8;
    	    modeinfo->greenWeight = 8;
    	    modeinfo->redWeight = 8;
    	    break;
    }
    modeinfo->bitsPerPixel = modeinfo->bytesPerPixel * 8;
    if (info->colors == 16)
	modeinfo->bitsPerPixel = 4;
    modeinfo->lineWidth = info->xbytes;
    return modeinfo;
}

/* Color modes. */
/* from ramdac/ramdac.h */
#define CLUT8_6		0
#define CLUT8_8		1
#define RGB16_555	2
#define RGB16_565	3
#define RGB24_888_B	4	/* 3 bytes per pixel, blue byte first. */
#define RGB32_888_B	5	/* 4 bytes per pixel. */


/*
 * This function converts a number of significant color bits to a matching
 * DAC mode type as defined in the RAMDAC interface.
 */

int __svgalib_colorbits_to_colormode(int bpp, int colorbits)
{
    if (colorbits == 8)
	return CLUT8_6;
    if (colorbits == 15)
	return RGB16_555;
    if (colorbits == 16)
	return RGB16_565;
    if (colorbits == 24) {
	if (bpp == 24)
	    return RGB24_888_B;
	else
	    return RGB32_888_B;
    }
    return CLUT8_6;
}

