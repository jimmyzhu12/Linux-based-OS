/* 
 * page.h - Defines used for paging
 * 
 * vim:ts=4 noexpandtab
 * 
 * "Copyright (c) 2022 by Hanwen Liu."
 * 
 * Author:        Hanwen Liu
 * Version:       1
 * Creation Date: Sat Mar  19 02:46:07 2022
 * Filename:      page.h
 * History:
 *    HL    1    Sat Mar  19 02:46:07 2022
 *        First written.
 */

#ifndef _PAGING_H
#define _PAGING_H

#include "types.h"

/* useful memory size */
#define SIZE_4KB			0x1000		/* 4 * (1 << 10) */
#define SIZE_4MB			0x400000	/* 4 * (1 << 20) */

/* define the number of the entries in each table */
#define NUM_PDE				1024
#define NUM_PTE				1024

/* kernel page -- 4-8MB */
#define KERNEL_ST			0x400000	// 4MB
#define KERNEL_END			0x800000	// 8MB

/* user page -- 128-132MB */
#define USER_ST				0x8000000	//128MB
#define USER_END			0x8400000	//132MB
#define _USER_ESP_			(USER_ST + SIZE_4MB - 4)

/* video page -- 256-260MB */
#define VIDEO_ST			0x10000000	//256MB
#define VIDEO_END			0x10400000	//260MB

/* shift offset when calculating the entries */
#define SHIFT_4MB 			22			/* for 4MB shift */
#define SHIFT_4KB			12			/* for 4KB shift */

// cp5: multi-terminals
#define TERMINAL_VIDEO_1	(VIDEO + SIZE_4KB)
#define TERMINAL_VIDEO_2	(VIDEO + (SIZE_4KB * 2))
#define TERMINAL_VIDEO_3	(VIDEO + (SIZE_4KB * 3))

union page_directory_entry {
	uint32_t val;
	struct { // 32 bits 
		uint32_t p		 : 1; 	/* present				 	*/
		uint32_t r_w 	 : 1; 	/* read/write 			 	*/
		uint32_t u_s	 : 1; 	/* user/supervisor		 	*/
		uint32_t pwt 	 : 1; 	/* write-through			*/
		uint32_t pcd	 : 1; 	/* cache disabled			*/
		uint32_t a	 	 : 1; 	/* accessed				 	*/
		uint32_t _0		 : 1; 	/* reserved(set to 0)		*/
		uint32_t ps 	 : 1; 	/* page size(0 -> 4KB) 	 	*/
		uint32_t g		 : 1; 	/* global page(ignored)		*/
		uint32_t avail 	 : 3;	/* reserved for programmer 	*/
		uint32_t pt_base :20;	/* page-table base address 	*/
	} __attribute__ ((__packed__)) _4KB;
	struct { 
		uint32_t p		 : 1; 	/* present				 	*/
		uint32_t r_w 	 : 1; 	/* read/write 			 	*/
		uint32_t u_s	 : 1; 	/* user/supervisor		 	*/
		uint32_t pwt 	 : 1; 	/* write-through			*/
		uint32_t pcd	 : 1; 	/* cache disabled			*/
		uint32_t a	 	 : 1; 	/* accessed				 	*/
		uint32_t d		 : 1; 	/* dirty					*/
		uint32_t ps 	 : 1; 	/* page size(1 -> 4MB) 	 	*/
		uint32_t g		 : 1; 	/* global page				*/
		uint32_t avail 	 : 3;	/* reserved for programmer 	*/
		uint32_t pat 	 : 1;	/* page table attribute idx */
		uint32_t rev 	 : 9;	/* reserved 				*/
		uint32_t p_base  :10;	/* page base address 		*/
	} __attribute__ ((__packed__)) _4MB;
};
typedef union page_directory_entry pde_t;


struct page_table_entry {
	uint32_t p		 : 1; 	/* present				  	*/
	uint32_t r_w 	 : 1;	/* read/write 			  	*/
	uint32_t u_s	 : 1;	/* user/supervisor		  	*/
	uint32_t pwt 	 : 1;	/* write-through			*/
	uint32_t pcd	 : 1;	/* cache disabled			*/
	uint32_t a	 	 : 1;	/* accessed				  	*/
	uint32_t d		 : 1;	/* dirty					*/
	uint32_t pat 	 : 1;	/* page table attribute idx */
	uint32_t g		 : 1;	/* global page			  	*/
	uint32_t avail 	 : 3;	/* reserved for programmer  */
	uint32_t p_base  :20;	/* page base address 		*/    // holds the start addresss
} __attribute__ ((__packed__));
typedef struct page_table_entry pte_t;


pde_t pde[NUM_PDE] __attribute__ ((__aligned__ (SIZE_4KB)));
// pte is for the 0-4MB memory
pte_t pte[NUM_PTE] __attribute__ ((__aligned__ (SIZE_4KB)));
// pte_vid is for the video memory
pte_t pte_vid[NUM_PTE] __attribute__ ((__aligned__ (SIZE_4KB)));

// init paging func
extern void page_init(void);

// flush TLBs
extern void flush_TLBs(void);

#endif /* _PAGING_H */
