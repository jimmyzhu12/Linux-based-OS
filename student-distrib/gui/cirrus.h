/* Copyright (c) 2022 by Hanwen Liu. */

/* 
 * cirrus.h
 * 
 * tab:4
 * 
 * Author:        Hanwen Liu
 * Version:       1
 * Creation Date: Sat May 7   00:16:49  2022 
 * Filename:      cirrus.h
 * History:
 *    HL    1     Sat May 7   00:16:49  2022
 *        Final revision.
 * 
 */

#ifndef CIRRUS_H
#define CIRRUS_H

extern int cirrus_test(void);
extern int cirrus_setmode(int mode, int prv_mode);
extern void cirrus_setdisplaystart(int address);
extern void cirrus_setlogicalwidth(int width);
extern void cirrus_setlinear(int addr);

extern info_t* cur_vga_info;

#endif /* CIRRUS_H */
