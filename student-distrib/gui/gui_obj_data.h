/* Copyright (c) 2022 by Hanwen Liu. */

/* 
 * gui_obj_data.h
 * 
 * tab:4
 * 
 * Author:        Hanwen Liu
 * Version:       1
 * Creation Date: Sat May 7   00:16:49  2022 
 * Filename:      gui_obj_data.h
 * History:
 *    HL    1     Sat May 7   00:16:49  2022
 *        Final revision.
 * 
 */

#ifndef GUI_OBJ_DATA_H
#define GUI_OBJ_DATA_H

#include "gui.h"

extern void gui_obj_init();

#define VER_FRAME_WIDTH		6
#define UPPER_FRAME_HEIGHT	21
#define DOWN_FRAME_HEIGHT	4
#define BOTTOM_WIDTH        11

#define BOTTOM_GAP			15
#define BT_R_X_offset		6
#define BT_Y_X_offset		(BT_R_X_offset + BOTTOM_GAP)
// G stand for grey here, but we are not using green bottom so far
#define BT_G_X_offset		(BT_Y_X_offset + BOTTOM_GAP)
// Y offset should be (UPPER_FRAME_HEIGHT - BOTTOM_WIDTH) >> 1
#define BT_Y_offset			5


/* in case I am spelling this word differently lol */
#define __gui_btm_gray		__gui_btm_grey

//  elements in buffer
#define ELE_X				0
#define ELE_Y				1800
#define ELE_UNIT			100
#define CORNOR_UNIT			(50 + VER_FRAME_WIDTH)

#define VER_L_X				(0)
#define VER_L_Y				(ELE_Y)
#define VER_L_W				(VER_FRAME_WIDTH)
#define VER_L_H				(ELE_UNIT)

#define VER_R_X				(VER_L_X + VER_FRAME_WIDTH)
#define VER_R_Y				(ELE_Y)
#define VER_R_W				(VER_FRAME_WIDTH)
#define VER_R_H				(ELE_UNIT)

#define UL_X				(VER_R_X + VER_R_W)
#define UL_Y				(ELE_Y)
#define UL_W				(CORNOR_UNIT)
#define UL_H				(UPPER_FRAME_HEIGHT)
#define UR_X				(UL_X)
#define UR_Y				(UL_Y + UL_H)
#define UR_W				(CORNOR_UNIT)
#define UR_H				(UPPER_FRAME_HEIGHT)
#define DL_X				(UL_X)
#define DL_Y				(UR_Y + UR_H)
#define DL_W				(CORNOR_UNIT)
#define DL_H				(DOWN_FRAME_HEIGHT)
#define DR_X				(UL_X)
#define DR_Y				(DL_Y + DL_H)
#define DR_W				(CORNOR_UNIT)
#define DR_H				(DOWN_FRAME_HEIGHT)

#define HOR_U_X				(UL_X + UL_W)
#define HOR_U_Y				(ELE_Y)
#define HOR_U_W				(ELE_UNIT)
#define HOR_U_H				(UPPER_FRAME_HEIGHT)
#define HOR_D_X				(HOR_U_X)
#define HOR_D_Y				(HOR_U_Y + HOR_U_H)
#define HOR_D_W				(ELE_UNIT)
#define HOR_D_H				(DOWN_FRAME_HEIGHT)

// G stand for grey. R stands for red. Y stands for yellow
#define BT_G_X				(HOR_U_X + HOR_U_W)
#define BT_G_Y				(ELE_Y)
#define BT_R_X				(BT_G_X)
#define BT_R_Y				(BT_G_Y + BOTTOM_WIDTH)
#define BT_Y_X				(BT_R_X)
#define BT_Y_Y				(BT_R_Y + BOTTOM_WIDTH)

extern vga_color __gui_btm_red[BOTTOM_WIDTH * BOTTOM_WIDTH];
extern vga_color __gui_btm_yellow[BOTTOM_WIDTH * BOTTOM_WIDTH];
extern vga_color __gui_btm_grey[BOTTOM_WIDTH * BOTTOM_WIDTH];
extern vga_color __gui_frm_UL[UL_W * UL_H];
extern vga_color __gui_frm_UR[UR_W * UR_H];
extern vga_color __gui_frm_DL[DL_W * DL_H];
extern vga_color __gui_frm_DR[DR_W * DR_H];
extern vga_color __gui_frm_U_unit[HOR_U_W * HOR_U_H];
extern vga_color __gui_frm_D_unit[HOR_D_W * HOR_D_H];
extern vga_color __gui_frm_L_unit[VER_L_W * VER_L_H];
extern vga_color __gui_frm_R_unit[VER_R_W * VER_R_H];


#endif /* GUI_OBJ_DATA_H */
