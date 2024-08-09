/* Copyright (c) 2022 by Hanwen Liu. */

/* 
 * interface.h
 * 
 * tab:4
 * 
 * Author:        Hanwen Liu
 * Version:       1
 * Creation Date: Sat May 7   00:16:49  2022 
 * Filename:      interface.h
 * History:
 *    HL    1     Sat May 7   00:16:49  2022
 *        Adaptaed from interface.h of SVGAlib 1.4.3. Final revision.
 *        Fix all warnings.
 * 
 */

#include "timing.h"

/* Prototypes of functions defined in interface.c. */

/*
 * This is a temporary function that allocates and fills in a ModeInfo
 * structure based on a svgalib mode number.
 */

ModeInfo * __svgalib_createModeInfoStructureForSvgalibMode(info_t* info);

/*
 * This function converts a number of significant color bits to a matching
 * DAC mode type as defined in the RAMDAC interface.
 */

int __svgalib_colorbits_to_colormode(int bpp, int colorbits);
