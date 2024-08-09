/* Copyright (c) 2022 by Hanwen Liu. */

/* 
 * gui.c
 * 
 * tab:4
 * 
 * Author:        Hanwen Liu
 * Version:       1
 * Creation Date: Sat May 7   00:16:49  2022 
 * Filename:      gui.c
 * History:
 *    HL    1     Sat May 7   00:16:49  2022
 *        Final revision. Fix warnings.
 * 
 */

#include "../macros.h"
#include "gui.h"
#include "fonts.h"
#include "../page.h"
#include "../cp5.h"
#include "vga.h"
#include "gui_obj_data.h"
#include "macros.h"

#define MOUSE_X_DIM		0x0A
#define MOUSE_Y_DIM		0x10
#define _32MB			(32 << 20)		// 32MB
#define DESKTOP_ST		(36 << 20)		// 36MB
#define DESKTOP_RD_ST	(40 << 20)		// 40MB
#define DESKTOP_GR_ST	(41 << 20)		// 41MB
#define DESKTOP_BL_ST	(42 << 20)		// 42MB

#ifdef _15B_COLOR
	#define R			((vga_color) 0x7C00)
	#define G			((vga_color) 0x03E0)
	#define B			((vga_color) 0x001F)
	#define SHIFT_R		10
	#define SHIFT_G		5
	#define SHIFT_B		0
#endif

/* the mask width is 2 * R + 1 */
#define	GBLUR_R			2
#define	GBLUR_MTX_W		(2 * GBLUR_R + 1)

/* (bytes) Other places may not using this macro */
#define VGA_PAGE_SIZE	(64 * 1024)

#define VIDEO_BASE		(VIDEO + SCREEN_SIZE)

void vga_memcpy(void* dest, const void* src, uint32_t n);

char __mouse_pattern[MOUSE_X_DIM * MOUSE_Y_DIM] = {
	//0123456789
	 "2         " // 0
	 "22        " // 1
	 "212       " // 2
	 "2112      " // 3
	 "21112     " // 4
	 "211112    " // 5
	 "2111112   " // 6
	 "21111112  " // 7
	 "211221112 " // 8
	 "2112 22222" // 9
	 "2112    22" // A
	 "212      2" // B
	 "212       " // C
	 "22        " // D
	 "22        " // E
	 "2         " // F
};

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define FAST_RENDER(fx, fy, tx, ty, w, h) {							\
	__svgalib_cirrusaccel_mmio_ScreenCopy(fx, fy, tx, ty, w, h);	\
}

#define SYNC_RENDER	__svgalib_cirrusaccel_mmio_Sync
		
extern unsigned int cur_terminal;   // 0, 1 or 2, this is the displaying terminal
extern unsigned int run_terminal;   // 0, 1 or 2, this is the running terminal.

extern void __svgalib_cirrusaccel_mmio_ScreenCopy(int x1, int y1, int x2, int y2,
												  int width, int height);
extern void cirrus_setdisplaystart(int address);
extern void __svgalib_cirrusaccel_mmio_Sync(void);
extern int get_mouse_pos(int* x, int* y);
extern int mouse_left_pressed;			/* pressed iff != 0 */

static void fill_desktop();
static void blur_status_bar();

// .981815
// static double __matrix[GBLUR_MTX_W * GBLUR_MTX_W] = {
// 	0.00291503, 0.01306425, 0.02153930, 0.01306425, 0.00291503, 
// 	0.01306425, 0.05854987, 0.09653238, 0.05854987, 0.01306425, 
// 	0.02153930, 0.09653238, 0.15915494, 0.09653238, 0.02153930, 
// 	0.01306425, 0.05854987, 0.09653238, 0.05854987, 0.01306425, 
// 	0.00291503, 0.01306425, 0.02153930, 0.01306425, 0.00291503
// };

// .357048
static double __matrix[GBLUR_MTX_W * GBLUR_MTX_W] = {
	.01133856, .01339492, .01416014, .01339492, .01133856,
	.01339492, .01582423, .01672823, .01582423, .01339492,
	.01416014, .01672823, .01768388, .01672823, .01416014,
	.01339492, .01582423, .01672823, .01582423, .01339492,
	.01133856, .01339492, .01416014, .01339492, .01133856
};

typedef struct Gaussion_blur_t {
	int r;
	double* matrix;
	double sum;			/* sum of matrix */
}gblur;

gblur mask = {GBLUR_R, __matrix, .357048};

/* We are double-buffering the output!
 * Either 0 or MAX_VGA_Y_DIM.
 */
static int cur_y_base = 0;
/* Inticates the max number of chars per line in current VGA mode */
static int char_per_line;
static int cur_xdim, cur_ydim, cur_xbytes;

gui_ter_t gui_terminal[NUMBER_OF_TERMINAL + 2];

void svga_test();
void vga_draw_pixel(unsigned int x, unsigned int y, vga_color color);

void set_gui_cur_vga_info() {
	cur_xdim = cur_vga_info->xdim;
	cur_ydim = cur_vga_info->ydim;
	cur_xbytes = cur_vga_info->xbytes;
}

void fast_render_inrange(int fx, int fy, int tx, int ty, int w, int h) {
	if (tx + w > 0 && tx < cur_xdim
					&& ty - cur_y_base >= 0
					&& ty - cur_y_base < cur_ydim)
	{
		if (tx >= 0) {
			__svgalib_cirrusaccel_mmio_ScreenCopy(fx, fy, tx, ty,
				MIN(cur_xdim - tx, w),
				MIN(cur_ydim + cur_y_base - ty, h));
		} else {
			tx = -tx;
			__svgalib_cirrusaccel_mmio_ScreenCopy(fx + tx, fy, 0, ty,
				(w - tx),
				MIN(cur_ydim + cur_y_base - ty, h));
		}
	}	
}

void gui_terminel_init() {
	int i;
	for (i = 0; i < 3; ++i) {
		gui_terminal[i].visible = 1;
		gui_terminal[i].priority = 2 - i; // 0 is the at the behind, 2 is on the top
		gui_terminal[i].x = 0;
		gui_terminal[i].y = 30;
	}
}

void font_obj_init(int font_id) {
#define FONT_NUM	3
	if (font_id >= 3 || font_id < -1) return;
	unsigned int c;
	int i, j;
	int x, y;
	if (font_id != -1) {
		font_height = font_select[font_id].h;
		font_width  = font_select[font_id].w;
		font = font_select[font_id].font;
	} 
	// else {
	// 	int tmp_f_id;
	// 	for (tmp_f_id = 0; tmp_f_id < 3; ++i) {
	// 		if (font_select) ;
	// 	}
	// }
	char_per_line = cur_xdim / font_width;
	int lucky = rand() % 10;
	if (font_id ==  1 && lucky != 0) {
		for (c = 0; c < 256; ++c) {
			x = (c % char_per_line) * font_width;
			y = MAX_VGA_Y_DIM * 2 + (c / char_per_line) * font_height;
			for (i = 0; i < font_width; ++i) {
				for (j = 0; j < font_height; ++j) {
					if (font[c * (font_width >> 3) * font_height + j + (i >> 3)] & (1 << i)) {
						vga_draw_pixel(i + x, j + y, FONT_ON);
					} else {
						vga_draw_pixel(i + x, j + y, FONT_OFF);
					}
				}
			}
		}
	} else {
		for (c = 0; c < 256; ++c) {
			x = (c % char_per_line) * font_width;
			y = MAX_VGA_Y_DIM * 2 + (c / char_per_line) * font_height;
			for (i = 0; i < font_width; ++i) {
				for (j = 0; j < font_height; ++j) {
					if (font[c * (font_width >> 3) * font_height + j + (i >> 3)] & (1 << (7 - (i & 0x7)))) {
						vga_draw_pixel(i + x, j + y, FONT_ON);
					} else {
						vga_draw_pixel(i + x, j + y, FONT_OFF);
					}
				}
			}
		}
	}
}

static vga_color moom[16 * 16] = {
	/* start line 0 */ 
	0x7fff, 0, 0, 0, 0x318c, 0x62f7, 0x62f7, 0x2529, 0x358c, 0x3dce, 0x0, 0, 0x0, 0x0, 0, 0, 
	/* start line 1 */ 
	0, 0, 0, 0x4e73, 0x56b5, 0x6f7b, 0x5293, 0x4210, 0x7fbe, 0x7bbd, 0, 0x0, 0, 0x0, 0x0, 0x0, 
	/* start line 2 */ 
	0, 0, 0x4e52, 0x4a31, 0x779c, 0x7bde, 0x7bde, 0x6f7b, 0x2929, 0, 0x0, 0x0, 0x0, 0x0, 0x0, 0, 
	/* start line 3 */ 
	0, 0x1cc6, 0x5694, 0x5ed6, 0x7bbd, 0x7bde, 0x6f7b, 0x14a5, 0x0, 0x0, 0, 0x0, 0x0, 0x0, 0x0, 0x0, 
	/* start line 4 */ 
	0x400, 0x3dee, 0x56b4, 0x7bbd, 0x7bdd, 0x7bdd, 0x2929, 0, 0x0, 0x0, 0x0, 0x0, 0x0, 0, 0x0, 0x0, 
	/* start line 5 */ 
	0x421, 0x6739, 0x77bd, 0x77bc, 0x7bde, 0x6b5a, 0x1ce7, 0x841, 0x0, 0, 0x0, 0x0, 0x0, 0x6739, 0x4631, 0x1084, 
	/* start line 6 */ 
	0xc42, 0x779c, 0x7bde, 0x6b39, 0x7fde, 0x6317, 0x842, 0, 0, 0x0, 0x0, 0x0, 0x0, 0x6f5b, 0x7bbe, 0x6739, 
	/* start line 7 */ 
	0x2508, 0x737b, 0x739b, 0x6f7b, 0x7ffe, 0x318c, 0x400, 0, 0, 0x0, 0x0, 0x0, 0x2108, 0x77bd, 0x7bde, 0x5ef7, 
	/* start line 8 */ 
	0x20e7, 0x737b, 0x7bde, 0x7bbd, 0x7bde, 0x5ef7, 0x421, 0x0, 0x0, 0x0, 0x0, 0x39ce, 0x77bd, 0x779d, 0x77bd, 0x56b5, 
	/* start line 9 */ 
	0x821, 0x5ed6, 0x739c, 0x737c, 0x7fff, 0x7bde, 0x4611, 0x0, 0x0, 0x0, 0x421, 0x6718, 0x7bde, 0x7fff, 0x7bde, 0x35ad, 
	/* start line 10 */ 
	0x400, 0x358c, 0x739c, 0x7bde, 0x77bd, 0x7fff, 0x7fde, 0x6739, 0x3dce, 0x35ad, 0x4631, 0x77bd, 0x7bde, 0x7bbd, 0x77bd, 0x400, 
	/* start line 11 */ 
	0x400, 0x1063, 0x6318, 0x6b5a, 0x7bde, 0x7fff, 0x7fff, 0x7bbe, 0x77bd, 0x7fff, 0x7bde, 0x77bd, 0x7bbe, 0x7bde, 0x4e73, 0x0, 
	/* start line 12 */ 
	0, 0x0, 0x39ad, 0x6f5b, 0x6739, 0x6b5a, 0x7fff, 0x7fde, 0x6739, 0x7bde, 0x77bd, 0x77bd, 0x77bd, 0x6318, 0x421, 0x0, 
	/* start line 13 */ 
	0x0, 0x0, 0x0, 0x3dcf, 0x6318, 0x5ef7, 0x6739, 0x7fff, 0x7fff, 0x7bde, 0x7fff, 0x77bd, 0x39ce, 0x0, 0x0, 0x0, 
	/* start line 14 */ 
	0, 0, 0x0, 0x0, 0x842, 0x1ce7, 0x2529, 0x318c, 0x739c, 0x7fff, 0x7bde, 0x2529, 0x0, 0x0, 0x0, 0x0, 
	/* start line 15 */ 
	0x4e52, 0, 0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x421, 0xc63, 0x0, 0x0, 0x0, 0x0, 0x5ef7
};

void get_desktop() {
	set_gui_cur_vga_info();

	int col;
	char* p = (char*)_32MB;

	for (col = 0; col < cur_ydim; ++col) {
		vga_memcpy(p, (void *)(DESKTOP_ST + col * MAX_VGA_X_WIDTH), cur_xbytes);
		p += cur_xbytes;
	}
}

void desktop_init() {

	pde[_32MB >> 22]._4MB.p 	= 1;		/* present!				*/
	pde[_32MB >> 22]._4MB.r_w   = 1;		/* always r/w avaliable	*/
	pde[_32MB >> 22]._4MB.u_s   = 0;
	pde[_32MB >> 22]._4MB.pwt   = 0;
	pde[_32MB >> 22]._4MB.pcd   = 0;
	pde[_32MB >> 22]._4MB.a 	= 0;
	pde[_32MB >> 22]._4MB.d 	= 0;
	pde[_32MB >> 22]._4MB.ps 	= 1; 		/* page size(1 -> 4MB)	*/
	pde[_32MB >> 22]._4MB.g		= 0;
	pde[_32MB >> 22]._4MB.avail = 0;
	pde[_32MB >> 22]._4MB.pat	= 0;
	pde[_32MB >> 22]._4MB.rev	= 0;
			   					/* map the page to its physical address */
	pde[_32MB >> 22]._4MB.p_base = _32MB >> 22;

	pde[DESKTOP_ST >> 22]._4MB.p 	  = 1;		/* present!				*/
	pde[DESKTOP_ST >> 22]._4MB.r_w    = 1;		/* always r/w avaliable	*/
	pde[DESKTOP_ST >> 22]._4MB.u_s    = 0;
	pde[DESKTOP_ST >> 22]._4MB.pwt    = 0;
	pde[DESKTOP_ST >> 22]._4MB.pcd    = 0;
	pde[DESKTOP_ST >> 22]._4MB.a 	  = 0;
	pde[DESKTOP_ST >> 22]._4MB.d 	  = 0;
	pde[DESKTOP_ST >> 22]._4MB.ps 	  = 1; 		/* page size(1 -> 4MB)	*/
	pde[DESKTOP_ST >> 22]._4MB.g	  = 0;
	pde[DESKTOP_ST >> 22]._4MB.avail  = 0;
	pde[DESKTOP_ST >> 22]._4MB.pat	  = 0;
	pde[DESKTOP_ST >> 22]._4MB.rev	  = 0;
			   					/* map the page to its physical address */
	pde[DESKTOP_ST >> 22]._4MB.p_base = DESKTOP_ST >> 22;

	pde[DESKTOP_RD_ST >> 22]._4MB.p 	= 1;	/* present!				*/
	pde[DESKTOP_RD_ST >> 22]._4MB.r_w   = 1;	/* always r/w avaliable	*/
	pde[DESKTOP_RD_ST >> 22]._4MB.u_s   = 0;
	pde[DESKTOP_RD_ST >> 22]._4MB.pwt   = 0;
	pde[DESKTOP_RD_ST >> 22]._4MB.pcd   = 0;
	pde[DESKTOP_RD_ST >> 22]._4MB.a 	= 0;
	pde[DESKTOP_RD_ST >> 22]._4MB.d 	= 0;
	pde[DESKTOP_RD_ST >> 22]._4MB.ps 	= 1; 	/* page size(1 -> 4MB)	*/
	pde[DESKTOP_RD_ST >> 22]._4MB.g		= 0;
	pde[DESKTOP_RD_ST >> 22]._4MB.avail = 0;
	pde[DESKTOP_RD_ST >> 22]._4MB.pat	= 0;
	pde[DESKTOP_RD_ST >> 22]._4MB.rev	= 0;
			   					/* map the page to its physical address */
	pde[DESKTOP_RD_ST >> 22]._4MB.p_base = DESKTOP_RD_ST >> 22;

	flush_TLBs();

	fill_desktop();

	blur_status_bar();

	int i, j;
	for (i = 0; i < 16; ++i) {
		for (j = 0; j < 16; ++j) {
			if (moom[j * 16 + i] != 0) {
				*((vga_color*)DESKTOP_ST + (j + 2) * MAX_VGA_X_DIM + 2 + i) = R | G | B;
			}
		}
	}
}

void vga_draw_pixel(unsigned int x, unsigned int y, vga_color color) {
	if (x >= cur_xdim) {
		return;
	}
	unsigned long offset = y * cur_xbytes + x * BYTESPERPIXEL;
	vga_setpage(offset >> 16);
	offset &= 0xFFFF;
#ifdef _15B_COLOR
	gr_writew(color, offset);
#endif
}

void render_desktop() {
	// int tmp;
	// for (tmp = cur_y_base;
	// 	 tmp + ELE_UNIT < cur_y_base + cur_ydim;
	// 	 tmp += ELE_UNIT)
	// {
	// 	fast_render_inrange(0, ELE_Y + ELE_UNIT,
	// 		0, tmp,
	// 		cur_xdim, ELE_UNIT);
	// }
	// fast_render_inrange(0, ELE_Y + ELE_UNIT,
	// 	0, tmp,
	// 	cur_xdim, cur_y_base + cur_ydim - tmp);

    int p, copy_st, copy_ed;
	int cur_display_st = cur_y_base * cur_xbytes;
	int cur_display_ed = (cur_y_base + cur_ydim) * cur_xbytes;
    for (p = 0; p < 2 * MAX_VGA_X_WIDTH * MAX_VGA_Y_DIM / VGA_PAGE_SIZE; ++p) {
		copy_st = MAX(cur_display_st, p * VGA_PAGE_SIZE);
		copy_ed = MIN(cur_display_ed, (p + 1) * VGA_PAGE_SIZE);
		if (copy_st < copy_ed) {
			vga_setpage(p);
			vga_memcpy((void *)(GM + (copy_st & (VGA_PAGE_SIZE - 1))), // GM + offset
			           (void *)(_32MB + copy_st - cur_y_base * cur_xbytes), // desktop source is not doubled
					   copy_ed - copy_st);
		}
    }
}

static unsigned int find_top_terminal() {
	int top_t_id, t_id = 0;
	// find a visible terminal
	for (t_id = 0; t_id < NUMBER_OF_TERMINAL && gui_terminal[t_id].visible == 0; ++t_id);
	top_t_id = t_id;
	for (; t_id < NUMBER_OF_TERMINAL && gui_terminal[t_id].visible == 1; ++t_id) {
		if (gui_terminal[t_id].priority > gui_terminal[top_t_id].priority) {
			top_t_id = t_id;
		}
	}
	return top_t_id;
}

void render_terminal_text(int t_id, int x, int y) {
	int row, col;
	for (col = 0; col < NUM_COLS; ++col) {
		for (row = 0; row < NUM_ROWS; ++row) {
			unsigned char c = *((unsigned char*)(VIDEO_BASE + SCREEN_SIZE * t_id) +
								(row * NUM_COLS + col) * 2);
			if (c == ' ') continue;
			int from_x, from_y, to_x, to_y;
			from_x = (c % char_per_line) * font_width;
			from_y = MAX_VGA_Y_DIM * 2 + (c / char_per_line) * font_height;
			to_x = x + col * font_width;
			to_y = y + cur_y_base + row * font_height;
			fast_render_inrange(from_x, from_y, to_x, to_y, font_width, font_height);
		}
	}
}

void render_terminal() {
	int i, j;
	for (i = 0; i < NUMBER_OF_TERMINAL; ++i) {
		for (j = 0; j < NUMBER_OF_TERMINAL; ++j) {
			if (gui_terminal[j].priority == i) {
				if (gui_terminal[j].visible != 0) {
					int tmp;
					// Render upper left cornor of the frame
					fast_render_inrange(UL_X, UL_Y,
						gui_terminal[j].x, cur_y_base + gui_terminal[j].y,
						UL_W, UL_H);
					// Render upper right cornor of the frame
					fast_render_inrange(UR_X, UR_Y,
						gui_terminal[j].x + TERMINAL_WIDTH - UR_W,
						cur_y_base + gui_terminal[j].y,
						UR_W, UR_H);
					// Render down left cornor of the frame
					fast_render_inrange(DL_X, DL_Y,
						gui_terminal[j].x,
						cur_y_base + gui_terminal[j].y + TERMINAL_HEIGHT - DL_H,
						DL_W, DL_H);
					// Render down right cornor of the frame
					fast_render_inrange(DR_X, DR_Y,
						gui_terminal[j].x + TERMINAL_WIDTH - DR_W,
						cur_y_base + gui_terminal[j].y + TERMINAL_HEIGHT - DR_H,
						DR_W, DR_H);
					// Render top and bottom edge of the frame
					// I met a problem without aa, bb, and cc. I don't know why!!
					int aa, bb, cc;
					for (tmp = (aa = gui_terminal[j].x + UL_W);
						 (bb = tmp + ELE_UNIT) < (cc = gui_terminal[j].x + TERMINAL_WIDTH - UR_W);
						 tmp += ELE_UNIT)
					{
						fast_render_inrange(HOR_U_X, HOR_U_Y,
							tmp, cur_y_base + gui_terminal[j].y,
							HOR_U_W, HOR_U_H);
						fast_render_inrange(HOR_D_X, HOR_D_Y,
							tmp, cur_y_base + gui_terminal[j].y + TERMINAL_HEIGHT - DR_H,
							HOR_D_W, HOR_D_H);
					}
					fast_render_inrange(HOR_U_X, HOR_U_Y,
						tmp, cur_y_base + gui_terminal[j].y,
						gui_terminal[j].x + TERMINAL_WIDTH - UR_W - tmp, HOR_U_H);
					fast_render_inrange(HOR_D_X, HOR_D_Y,
						tmp, cur_y_base + gui_terminal[j].y + TERMINAL_HEIGHT - DR_H,
						gui_terminal[j].x + TERMINAL_WIDTH - DR_W - tmp, HOR_D_H);
					// Render left and right edge of the frame
					for (tmp = cur_y_base + gui_terminal[j].y + UPPER_FRAME_HEIGHT;
						 tmp + ELE_UNIT < cur_y_base + gui_terminal[j].y + TERMINAL_HEIGHT - DOWN_FRAME_HEIGHT;
						 tmp += ELE_UNIT)
					{
						fast_render_inrange(VER_L_X, VER_L_Y,
							gui_terminal[j].x, tmp, VER_L_W, VER_L_H);
						fast_render_inrange(0, ELE_Y + ELE_UNIT,
							gui_terminal[j].x + VER_FRAME_WIDTH, tmp,
							NUM_COLS * font_width, ELE_UNIT);
						fast_render_inrange(VER_R_X, VER_R_Y,
							gui_terminal[j].x + TERMINAL_WIDTH - VER_FRAME_WIDTH, tmp,
							VER_R_W, VER_R_H);
					}
					// Render the grey bottoms
					fast_render_inrange(BT_G_X, BT_G_Y,
						gui_terminal[j].x + BT_R_X_offset,
						cur_y_base + gui_terminal[j].y + BT_Y_offset,
						BOTTOM_WIDTH, BOTTOM_WIDTH);
					fast_render_inrange(BT_G_X, BT_G_Y,
						gui_terminal[j].x + BT_Y_X_offset,
						cur_y_base + gui_terminal[j].y + BT_Y_offset,
						BOTTOM_WIDTH, BOTTOM_WIDTH);
					fast_render_inrange(BT_G_X, BT_G_Y,
						gui_terminal[j].x + BT_G_X_offset,
						cur_y_base + gui_terminal[j].y + BT_Y_offset,
						BOTTOM_WIDTH, BOTTOM_WIDTH);
					// fill the interal as black!
					fast_render_inrange(VER_L_X, VER_L_Y,
						gui_terminal[j].x, tmp,
						VER_L_W, cur_y_base + gui_terminal[j].y + TERMINAL_HEIGHT - DOWN_FRAME_HEIGHT - tmp);
					fast_render_inrange(0, ELE_Y + ELE_UNIT,
						gui_terminal[j].x + VER_FRAME_WIDTH, tmp,
						NUM_COLS * font_width, cur_y_base + gui_terminal[j].y + TERMINAL_HEIGHT - DOWN_FRAME_HEIGHT - tmp);
					fast_render_inrange(VER_R_X, VER_R_Y,
						gui_terminal[j].x + TERMINAL_WIDTH - VER_FRAME_WIDTH, tmp,
						VER_R_W, cur_y_base + gui_terminal[j].y + TERMINAL_HEIGHT - DOWN_FRAME_HEIGHT - tmp);
					// Render the consept
					render_terminal_text(j,								\
						gui_terminal[j].x + VER_FRAME_WIDTH,			\
						gui_terminal[j].y + UPPER_FRAME_HEIGHT);
				}
				j = NUMBER_OF_TERMINAL; // jump out of j loop
			}
		}
	}
	// render the top terminal's bottom
	unsigned int top_t_id = find_top_terminal();
	if (top_t_id < NUMBER_OF_TERMINAL) {
		fast_render_inrange(BT_R_X, BT_R_Y,
			gui_terminal[top_t_id].x + BT_R_X_offset,
			cur_y_base + gui_terminal[top_t_id].y + BT_Y_offset,
			BOTTOM_WIDTH, BOTTOM_WIDTH);
		fast_render_inrange(BT_Y_X, BT_Y_Y,
			gui_terminal[top_t_id].x + BT_Y_X_offset,
			cur_y_base + gui_terminal[top_t_id].y + BT_Y_offset,
			BOTTOM_WIDTH, BOTTOM_WIDTH);
	}
}

void render_mouse() {
#define TWINK_MIN				0x07
#define TWINK_MAX				0x18
	int x, y;
	get_mouse_pos(&x, &y);
	static unsigned char test = TWINK_MIN;
	char c;
	int i, j;
	static int flag = 1;
	static const vga_color pressed  = (TWINK_MIN << SHIFT_R) | (TWINK_MIN << SHIFT_G) |  (TWINK_MIN << SHIFT_B);
	static const vga_color released = (TWINK_MAX << SHIFT_R) | (TWINK_MAX << SHIFT_G) |  (TWINK_MAX << SHIFT_B);
	for (j = MOUSE_Y_DIM - 1; j >= 0; --j) {
		for (i = MIN(MOUSE_X_DIM - 1, j); i >= 0; --i) {
			if (i + x < cur_xdim && j + y < cur_ydim) {
				c = __mouse_pattern[j * MOUSE_X_DIM + i];
				if (c == '2') {
					if (mouse_left_pressed != 0){
						vga_draw_pixel(i + x, cur_y_base + j + y, pressed);
					} else {
						vga_draw_pixel(i + x, cur_y_base + j + y, released);
					}
				} else if (c == '1') {
					vga_draw_pixel(i + x, cur_y_base + j + y, (test | (test << 5) | (test << 10)));
				}
			}
		}
	}
	x = rand() % 10;
	switch (x)
	{
	case 0:
		--test;
		break;
	case 1:
		test += flag;
		break;
	case 2:
		++test;
		break;
	default:
		return;
	}
	if (test > TWINK_MAX - 5) {
		flag = -1;
		test = MIN(test, TWINK_MAX);
	} else if (test < TWINK_MIN + 5) {
		flag = 1;
		test = MAX(test, TWINK_MIN);
	}
}

static void render_time() {
	real_time_t *cur_time = update_real_time_from_rtc();
	int secl = cur_time->second % 10;
	int sech = cur_time->second / 10;
	int minl = cur_time->minute % 10;
	int minh = cur_time->minute / 10;
	int hrl  = cur_time->hour % 10;
	int hrh  = cur_time->hour / 10;
	int cur_x = (cur_xdim - 84);
	const int cur_y = 2;
	int i, j;
	unsigned char c;
	for (i = 0; i < 8; ++i) {
		for (j = 0; j < 16; ++j) {
			c = hrh + '0';
			if (font_form_mp2[c][j] & (1 << (7 - i))) {
				vga_draw_pixel(i + cur_x, cur_y_base + j + cur_y, FONT_ON);
			}
		}
	}
	cur_x += 9;
	for (i = 0; i < 8; ++i) {
		for (j = 0; j < 16; ++j) {
			c = hrl + '0';
			if (font_form_mp2[c][j] & (1 << (7 - i))) {
				vga_draw_pixel(i + cur_x, cur_y_base + j + cur_y, FONT_ON);
			}
		}
	}
	cur_x += 9;
	if ((secl & 1) == 1) {
		for (i = 0; i < 8; ++i) {
			for (j = 0; j < 16; ++j) {
				if (font_form_mp2[':'][j] & (1 << (7 - i))) {
					vga_draw_pixel(i + cur_x, cur_y_base + j + cur_y, FONT_ON);
				}
			}
		}
	}
	cur_x += 9;
	for (i = 0; i < 8; ++i) {
		for (j = 0; j < 16; ++j) {
			c = minh + '0';
			if (font_form_mp2[c][j] & (1 << (7 - i))) {
				vga_draw_pixel(i + cur_x, cur_y_base + j + cur_y, FONT_ON);
			}
		}
	}
	cur_x += 9;
	for (i = 0; i < 8; ++i) {
		for (j = 0; j < 16; ++j) {
			c = minl + '0';
			if (font_form_mp2[c][j] & (1 << (7 - i))) {
				vga_draw_pixel(i + cur_x, cur_y_base + j + cur_y, FONT_ON);
			}
		}
	}
	cur_x += 9;
	if ((secl & 1) == 1) {
		for (i = 0; i < 8; ++i) {
			for (j = 0; j < 16; ++j) {
				if (font_form_mp2[':'][j] & (1 << (7 - i))) {
					vga_draw_pixel(i + cur_x, cur_y_base + j + cur_y, FONT_ON);
				}
			}
		}
	}
	cur_x += 9;
	for (i = 0; i < 8; ++i) {
		for (j = 0; j < 16; ++j) {
			c = sech + '0';
			if (font_form_mp2[c][j] & (1 << (7 - i))) {
				vga_draw_pixel(i + cur_x, cur_y_base + j + cur_y, FONT_ON);
			}
		}
	}
	cur_x += 9;
	for (i = 0; i < 8; ++i) {
		for (j = 0; j < 16; ++j) {
			c = secl + '0';
			if (font_form_mp2[c][j] & (1 << (7 - i))) {
				vga_draw_pixel(i + cur_x, cur_y_base + j + cur_y, FONT_ON);
			}
		}
	}
}

void gui_render() {
	cli();

	set_gui_cur_vga_info();

	// svga_test();
	// Switch view window
	cur_y_base = MAX_VGA_Y_DIM - cur_y_base;

	save_terminal_contents(run_terminal);

	// Render desktop
	render_desktop();

	// Render 24-hr clock
	render_time();

	// Render all the terminals
	render_terminal();

	// Render mouse
	render_mouse();

	// Spin until all operation is complete
	SYNC_RENDER();

	// Set to the new view window!
	cirrus_setdisplaystart(cur_y_base * cur_xbytes);
}

// void svga_test() {

//     int x, y;

//     for (x = 80; x < 110; x++) {
//         for (y = 50; y <= 80; y++) {
//             vga_draw_pixel(x, y, R|G|B);
//         }
//     }

//     for (x = 80; x < 110; x++) {
//         for (y = 110; y <= 140; y++) {
//             vga_draw_pixel(x, y, R|G);
//         }
//     }

//     for (x = 80; x < 110; x++) {
//         for (y = 170; y <= 200; y++) {
//             vga_draw_pixel(x, y, R|B);
//         }
//     }

//     for (x = 200; x < 230; x++) {
//         for (y = 50; y <= 80; y++) {
//             vga_draw_pixel(x, y, G|B);
//         }
//     }

//     for (x = 200; x < 230; x++) {
//         for (y = 110; y <= 140; y++) {
//             vga_draw_pixel(x, y, G);
//         }
//     }

//     for (x = 600; x < 660; x++) {
//         for (y = 170; y <= 200; y++) {
//             vga_draw_pixel(x, y, B);
//         }
//     }

//     for (x = 600; x < 660; x++) {
//         for (y = 250; y <= 450; y++) {
//             vga_draw_pixel(x, y, R);
//         }
//     }

//     for (x = 80; x < 700; x++) {
//         for (y = 350; y <= 550; y++) {
//             vga_draw_pixel(x, y, R|G|B);
//         }
//     }

//     FAST_RENDER(80, 50, 180, 50, 30, 200);

// 	FAST_RENDER(0, 2 * MAX_VGA_Y_DIM, 0, 400, cur_xdim, 100);
// 	FAST_RENDER(ELE_X, ELE_Y, 0, 560, cur_xdim, 100);
// 	while (1);
// }

void gui_obj_init() {
	int x, y;
	// for (x = 0; x < cur_xdim; ++x) {
    //     for (y = 0; y < 100; ++y) {
    //         vga_draw_pixel(ELE_X + x, ELE_Y + y, R);
    //     }
    // }

	vga_color c;

	for (x = 0; x < BOTTOM_WIDTH; ++x) {
		for (y = 0; y < BOTTOM_WIDTH; ++y) {
			c = __gui_btm_red[y * BOTTOM_WIDTH + x];
			vga_draw_pixel(x + BT_R_X, y + BT_R_Y, c);
			c = __gui_btm_grey[y * BOTTOM_WIDTH + x];
			vga_draw_pixel(x + BT_G_X, y + BT_G_Y, c);
			c = __gui_btm_yellow[y * BOTTOM_WIDTH + x];
			vga_draw_pixel(x + BT_Y_X, y + BT_Y_Y, c);
		}
	}
	for (x = 0; x < VER_R_W; ++x) {
		for (y = 0; y < VER_R_H; ++y) {
			c = __gui_frm_R_unit[y * VER_R_W + x];
			vga_draw_pixel(x + VER_R_X, y + VER_R_Y, c);
		}
	}
	for (x = 0; x < VER_L_W; ++x) {
		for (y = 0; y < VER_L_H; ++y) {
			c = __gui_frm_L_unit[y * VER_L_W + x];
			vga_draw_pixel(x + VER_L_X, y + VER_L_Y, c);
		}
	}
	for (x = 0; x < HOR_U_W; ++x) {
		for (y = 0; y < HOR_U_H; ++y) {
			c = __gui_frm_U_unit[y * HOR_U_W + x];
			vga_draw_pixel(x + HOR_U_X, y + HOR_U_Y, c);
		}
	}
	for (x = 0; x < HOR_D_W; ++x) {
		for (y = 0; y < HOR_D_H; ++y) {
			c = __gui_frm_D_unit[y * HOR_D_W + x];
			vga_draw_pixel(x + HOR_D_X, y + HOR_D_Y, c);
		}
	}
	for (x = 0; x < UL_W; ++x) {
		for (y = 0; y < UL_H; ++y) {
			c = __gui_frm_UL[y * UL_W + x];
			vga_draw_pixel(x + UL_X, y + UL_Y, c);
		}
	}
	for (x = 0; x < UR_W; ++x) {
		for (y = 0; y < UR_H; ++y) {
			c = __gui_frm_UR[y * UR_W + x];
			vga_draw_pixel(x + UR_X, y + UR_Y, c);
		}
	}
	for (x = 0; x < DL_W; ++x) {
		for (y = 0; y < DL_H; ++y) {
			c = __gui_frm_DL[y * DL_W + x];
			vga_draw_pixel(x + DL_X, y + DL_Y, c);
		}
	}
	for (x = 0; x < DR_W; ++x) {
		for (y = 0; y < DR_H; ++y) {
			c = __gui_frm_DR[y * DR_W + x];
			vga_draw_pixel(x + DR_X, y + DR_Y, c);
		}
	}
}

#define get_color(x, y) (*((vga_color *)DESKTOP_ST + y * MAX_VGA_X_DIM + x))

/* Magic Generator from Martin Büttner */
/* From https://codegolf.stackexchange.com/questions/35569/tweetable-mathematical-art */
#define r(n) (rand() % n)
#define get_rd(x, y) ((char *)DESKTOP_RD_ST + y * MAX_VGA_X_DIM + x)
#define get_gr(x, y) ((char *)DESKTOP_GR_ST + y * MAX_VGA_X_DIM + x)
#define get_bl(x, y) ((char *)DESKTOP_BL_ST + y * MAX_VGA_X_DIM + x)
unsigned char RD(int i,int j){
	return!*get_rd(i,j)? (*get_rd(i,j) = !r(999) ? r(64) : RD((i+r(2))%MAX_VGA_X_WIDTH,(j+r(2))%MAX_VGA_Y_DIM)) :*get_rd(i,j);
}
 
unsigned char GR(int i,int j){
	return!*get_gr(i,j)? (*get_gr(i,j) = !r(999) ? r(64) : GR((i+r(2))%MAX_VGA_X_WIDTH,(j+r(2))%MAX_VGA_Y_DIM)) :*get_gr(i,j);
}
 
unsigned char BL(int i,int j){
	return!*get_bl(i,j)? (*get_bl(i,j) = !r(999) ? r(64) : BL((i+r(2))%MAX_VGA_X_WIDTH,(j+r(2))%MAX_VGA_Y_DIM)) :*get_bl(i,j);
}

void draw_desktop_pixel(int i, int j) {
    static unsigned short r, g, b;
    r = RD(i,j) & 0x1F;
    g = GR(i,j) & 0x1F;
    b = BL(i,j) & 0x1F;
    get_color(i, j) = (r << SHIFT_R) | (g << SHIFT_G) | (b);
}

void fill_desktop() {
	real_time_t *cur_time = update_real_time_from_rtc();
	set_seed(cur_time->second + cur_time->minute);
		
	vga_memset((void *)DESKTOP_ST, 0, MAX_VGA_X_WIDTH * MAX_VGA_Y_DIM);
	vga_memset((void *)DESKTOP_RD_ST, 0, 3 * 1024 * 1024);
	int x, y;
	for (x = 0; x < MAX_VGA_X_WIDTH; ++x) {
		for (y = 0; y < MAX_VGA_Y_DIM; ++y) {
			draw_desktop_pixel(x, y);
		}
	}
}

#ifdef _15B_COLOR
	#define GRAY	0x10
#endif

void blur_status_bar() {
	int col, row, dx, dy, tmp_x, tmp_y;
	double tmp_para;
	double cur_r, cur_g, cur_b;
	vga_color tmp_r, tmp_g, tmp_b, tmp_color;
	for (col = 0; col < MAX_VGA_X_DIM; ++col) {
		for (row = 0; row < TOP_BAR_HEIGHT; ++row) {
#define get_gblur_para(x, y) (mask.matrix[(x + GBLUR_R) * GBLUR_MTX_W + (y + GBLUR_R)])
			cur_r = cur_g = cur_b = 0;
			for (dx = -GBLUR_R; dx <= GBLUR_R; ++dx) {
				for (dy = -GBLUR_R; dy <= GBLUR_R; ++dy) {
					tmp_x = col + dx;
					tmp_y = row + dy;
					if (tmp_x >= 0 && tmp_x < MAX_VGA_X_DIM &&
					    tmp_y >= 0 && tmp_y < MAX_VGA_Y_DIM)
					{
						tmp_color = get_color(tmp_x, tmp_y);
						tmp_para = get_gblur_para(dx, dy);
						cur_r += ((tmp_color & R) >> SHIFT_R) * tmp_para;
						cur_g += ((tmp_color & G) >> SHIFT_G) * tmp_para;
						cur_b += ((tmp_color & B) >> SHIFT_B) * tmp_para;
					} else {
						tmp_para = get_gblur_para(dx, dy);
						cur_r += GRAY * tmp_para;
						cur_g += GRAY * tmp_para;
						cur_b += GRAY * tmp_para;
					}
				}
			}
#define M	0x09
			tmp_r = ((vga_color)(cur_r / mask.sum) + M) >> 1;
			tmp_g = ((vga_color)(cur_g / mask.sum) + M) >> 1;
			tmp_b = ((vga_color)(cur_b / mask.sum) + M) >> 1;
#undef M
			get_color(col, row) = (tmp_r << SHIFT_R) | (tmp_g << SHIFT_G) | (tmp_b);
#undef get_gblur_para
		}
	}
}
#undef get_color

/* void* vga_memcpy(void* dest, const void* src, uint32_t n);
 * Inputs:      void* dest = destination of copy
 *         const void* src = source of copy
 *              uint32_t n = number of byets to copy
 * Return Value: none
 * Function: copy n bytes of src to dest */
void vga_memcpy(void* dest, const void* src, uint32_t n) {
    asm volatile ("                 \n\
            .memcpy_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memcpy_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memcpy_aligned \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memcpy_top     \n\
            .memcpy_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     movsl           \n\
            .memcpy_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memcpy_done    \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%edx       \n\
            jmp     .memcpy_bottom  \n\
            .memcpy_done:           \n\
            "
            :
            : "S"(src), "D"(dest), "c"(n)
            : "eax", "edx", "memory", "cc"
    );
}

#undef R
#undef G
#undef B

