/* Copyright (c) 2022 by Hanwen Liu. */

/* 
 * font.h
 * 
 * tab:4
 * 
 * Author:        Hanwen Liu
 * Version:       1
 * Creation Date: Sat May 7   00:16:49  2022 
 * Filename:      font.h
 * History:
 *    HL    1     Sat May 7   00:16:49  2022
 *        Final revision.
 * 
 */

#ifndef FONT_H
#define FONT_H

#include "gui.h"
#include "../macros.h"

#ifdef _15B_COLOR
	#define FONT_ON		((vga_color) 0x7FFF)
	#define FONT_OFF	((vga_color) 0x0000)
#elif
	// nothing
#endif

typedef struct font_descriptor {
	int w;
	int h;
	unsigned char* font;
}font_t;

extern unsigned int font_width;
extern unsigned int font_height;

extern unsigned char* font;

extern font_t font_select[];

extern unsigned char font_form_mp2[256][16];

#endif /* FONT_H */
