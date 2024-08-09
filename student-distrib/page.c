/* 
 * page.c - Defines used for paging
 * 
 * vim:ts=4 noexpandtab
 * 
 * "Copyright (c) 2022 by Hanwen Liu."
 * 
 * Author:        Hanwen Liu
 * Version:       1
 * Creation Date: Sat Mar  19 16:22:11 2022
 * Filename:      page.c
 * History:
 *    HL    1    Sat Mar  19 16:22:11 2022
 *        First written.
 */

#include "page.h"
#include "macros.h"

void page_enable(void);

/*
 * init_paging
 *   DESCRIPTION: Initializes paging when booting
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Paging is enabled and initialized
 */
void page_init(void) {
	uint32_t i;

	/* initialize the page-directory entry */
	for (i = 0; i < NUM_PDE; ++i) {
		pde[i]._4MB.p 	   = 0;		/* not present			*/
		pde[i]._4MB.r_w    = 1;		/* always r/w avaliable	*/
		pde[i]._4MB.u_s    = 0;
		pde[i]._4MB.pwt    = 0;
		pde[i]._4MB.pcd    = 0;
		pde[i]._4MB.a 	   = 0;
		pde[i]._4MB.d 	   = 0;
		pde[i]._4MB.ps 	   = 1; 	/* page size(1 -> 4MB)	*/
		pde[i]._4MB.g	   = 0;
		pde[i]._4MB.avail  = 0;
		pde[i]._4MB.pat	   = 0;
		pde[i]._4MB.rev	   = 0;
				   /* map each page to its physical address */
		pde[i]._4MB.p_base = i;
	}

	/* initialize the 0-4MB page-table entry */
	for (i = 0; i < NUM_PTE; ++i) {
		/* activate the video memory, which has mapped to the physical address */
									/* whether present?		*/
		pte[i].p	  = (i < (uint32_t)VIDEO >> SHIFT_4KB ? 0 : 1);
		pte[i].r_w	  = 1;			/* always r/w avaliable	*/
		pte[i].u_s	  = 0;
		pte[i].pwt	  = 0;
		pte[i].pcd	  = 0;
		pte[i].a	  = 0;
		pte[i].d	  = 0;
		pte[i].pat	  = 0;
		pte[i].g	  = 0;
		pte[i].avail  = 0;
				   /* map each page to its physical address */
		pte[i].p_base = i;
	}
	
	/* initialize the video page-table entry */
	for (i = 0; i < NUM_PTE; ++i) {
		pte_vid[i].p	  = 0;		/* not present			*/
		pte_vid[i].r_w	  = 1;		/* always r/w avaliable	*/
		pte_vid[i].u_s	  = 1;		/* visible to user		*/
		pte_vid[i].pwt	  = 0;
		pte_vid[i].pcd	  = 0;
		pte_vid[i].a	  = 0;
		pte_vid[i].d	  = 0;
		pte_vid[i].pat	  = 0;
		pte_vid[i].g	  = 0;
		pte_vid[i].avail  = 0;
		pte_vid[i].p_base = 0;
	}

	/* break the first 4MB of memory into 4KB pages */
	pde[0]._4KB.p   = 1;			/* present!				*/
	pde[0]._4KB.r_w = 1;			/* always r/w avaliable	*/
	pde[0]._4KB.ps  = 0;			/* page size(0 -> 4KB)	*/
						/* point to the page-table entry	*/
	pde[0]._4KB.pt_base = (uint32_t)pte >> SHIFT_4KB;

	/* map virtual memory 4-8MB to physical memory 4-8MB */
	pde[KERNEL_ST >> SHIFT_4MB]._4MB.p = 1;				/* present!		*/
	pde[KERNEL_ST >> SHIFT_4MB]._4MB.g = 1;				/* global page!	*/
										/* point to the address of 4MB	*/
	pde[KERNEL_ST >> SHIFT_4MB]._4MB.p_base = (uint32_t)KERNEL_ST >> SHIFT_4MB;

	// cp5. activate the video memories for the terminals
	pte[(uint32_t)TERMINAL_VIDEO_1 >> SHIFT_4KB].p = 1;
	pte[(uint32_t)TERMINAL_VIDEO_2 >> SHIFT_4KB].p = 1;
	pte[(uint32_t)TERMINAL_VIDEO_3 >> SHIFT_4KB].p = 1;
	
	/* enable paging */
	page_enable();
}

/*
 * page_enable
 *   DESCRIPTION: Enables the protected mode, paging, and 4MB page size.
 * 					Fills the Page Directory Base Register (CR3). 
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: See description.
 */
void page_enable(void) {
	__asm__ __volatile__ ("											   \n\
		/* load cr3 with the address of pde */						   \n\
		movl %0,	%%eax		/* eax <- pde						*/ \n\
		movl %%eax, %%cr3		/* cr3 <- eax						*/ \n\
																	   \n\
		/* set the bit 4 of cr4 (PSE) to 1 to and					   \n\
		 * set the bit 5 of cr4 (PAE) to 0 to enable 4MB pages */	   \n\
		movl %%cr4,	%%eax		/* eax <- cr4						*/ \n\
		orl  $0x00000010, %%eax /* set PSE to 1						*/ \n\
		andl $0xFFFFFFDF, %%eax /* set PAE to 0						*/ \n\
		movl %%eax, %%cr4		/* cr4 <- eax						*/ \n\
																	   \n\
		/* Are we using the Pentium processor? */					   \n\
		/* When enabling or disabling large page sizes,				   \n\
				the TLBs must be invalidated. */					   \n\
		call flush_TLBs												   \n\
																	   \n\
		/* set the bit 31 (PG) and the bit 0 (PE) of cr0 to 1 */	   \n\
		movl %%cr0,	%%eax		/* eax <- cr0						*/ \n\
		orl  $0x80000001, %%eax /* set the PG and PE bits to 1		*/ \n\
		movl %%eax, %%cr0		/* cr0 <- eax						*/ \n\
		"
		:						/* no outputs 						*/
		: "g"(pde)				/* input 0 is the address of pde	*/
		: "eax", "memory"		/* eax and part of memory are
									subject to be changed			*/
	);
}

/*
 * flush_TLBs
 *   DESCRIPTION: flushes translation lookaside buffers
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: TLBs are flushed
 */
void flush_TLBs() {
	/* TLBs flushed when cr3 is reloaded */
	__asm__ __volatile__ ("											   \n\
		movl %%cr3,	%%eax		/* eax <- cr3						*/ \n\
		movl %%eax, %%cr3		/* cr3 <- eax						*/ \n\
		"
		: :						/* no outputs and inputs			*/
		: "eax"					/* eax are subject to be changed	*/
    );
}
