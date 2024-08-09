/* Copyright (c) 2022 by Hanwen Liu. */

/* 
 * gui.h
 * 
 * tab:4
 * 
 * Author:        Hanwen Liu
 * Version:       1
 * Creation Date: Sat May 7   00:16:49  2022 
 * Filename:      gui.h
 * History:
 *    HL    1     Sat May 7   00:16:49  2022
 *        Final revision. Fix warnings.
 * 
 */

#ifndef GUI_H
#define GUI_H

#define _15B_COLOR

#ifdef _15B_COLOR
	/*
	 * ┌─15┬14─10┬9───5┬4───0┐
	 * │ 0 │  R  │  G  │  B  │
	 * └───┴─────┴─────┴─────┘
	 */
	typedef unsigned short vga_color;
	#define BYTESPERPIXEL	2					/* only support 15b color	*/
#elif
	typedef unsigned int vga_color; // should not use
#endif

/* 
 * X_DIM   is a horizontal screen dimension in pixels.
 * X_WIDTH is a horizontal screen dimension in 'natural' units
 *         (addresses, characters of text, etc.)
 * Y_DIM   is a vertical screen dimension in pixels.
 */

#define MAX_VGA_X_DIM	1024					/* max full image width		*/
												/* addresses (bytes)		*/
#define MAX_VGA_X_WIDTH	(MAX_VGA_X_DIM * BYTESPERPIXEL)
#define MAX_VGA_Y_DIM	768						/* max full image height	*/

#define TERMINAL_WIDTH	(NUM_COLS * font_width + 2 * VER_FRAME_WIDTH)
#define TERMINAL_HEIGHT	(NUM_ROWS * font_height + UPPER_FRAME_HEIGHT + DOWN_FRAME_HEIGHT)

typedef struct gui_terminal_t {
	int x;
	int y;
	int visible;			/* invisible iff 0 		*/
	unsigned int priority;  /* 0 is the very bottom */
}gui_ter_t;

extern gui_ter_t gui_terminal[];

extern void gui_render();

extern void gui_terminel_init();

extern void desktop_init();

extern void font_obj_init(int font_id);

#endif /* GUI_H */
