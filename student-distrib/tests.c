/* 
 * tests.c
 * 
 * vim:ts=4 noexpandtab
 * 
 * "Copyright (c) 2022 by Hanwen Liu."
 * 
 * Author:        Hanwen Liu
 * Version:       1
 * Creation Date: Sat Mar  21 20:42:55 2022
 * Filename:      tests.c
 * History:
 *    HL    1	  Sat Mar  21 20:42:55 2022
 *        First written, shamelessly modified from the ECD 391 MP3.
 */

#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "rtc.h"
#include "terminal.h"
#include "FileSystem.h"

/* cp3.1 test list */
void idt_test();
void basic_keyboard_test();
void irq_syscall_test();
void divide_zero_test();
void page_easy_test();
void page_fault_test();
void cp1_hybrid_test();

/* cp3.2 test list */
void terminal_easy_test();
void shell_len_test();
void spam_output_test();
void rtc_cp2_test();
// void list_all_files();
// void read_file_by_name();


static void (*test)(void) = idt_test;

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER					\
	printf("\n[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)

#define TEST_OUTPUT(result)			\
	printf("\n[TEST %s] Result = %s\n", __FUNCTION__, (result) ? "PASS" : "FAIL");

/* pulse for a period of time 100000000 might be 0.1s or so? */
#define _PULSE(time)				\
do {								\
	volatile uint32_t i;			\
	for (i = 0; i < time; ++i);		\
} while(0);							\

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
void idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	TEST_OUTPUT(result);
}

/* Basic Keyboard Test
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: lowercase letters and numbers
 * Files: Device/keyboard_handler.h/c
 */
void basic_keyboard_test() {
	TEST_HEADER;

	printf("Please press some key.\n");
	
    _PULSE(2000000000);
	
	printf("\nTest finished.\n");

	TEST_OUTPUT(PASS);
}

/* Basic RTC Test
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: RTC
 * Files: rtc.h/c
 */
void rtc_test() {
	TEST_HEADER;

	int i;

	printf("Enableing rtc test");
	for (i = 0; i < 6; ++i) {
	    _PULSE(50000000);
		printf(".");
	}
	printf("\n\n\n");
	rtc_test_enable();
	
	_PULSE(2000000000);

	rtc_test_disable();

	printf("Disableing rtc test");
	for (i = 0; i < 6; ++i) {
	    _PULSE(50000000);
		printf(".");
	}
	printf("\n\n\n");

	printf("\nTest finished.\n");

	TEST_OUTPUT(PASS);
}

/* System Call Test
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
void irq_syscall_test() {
	TEST_HEADER;
	
	asm volatile("int $0x80");

	TEST_OUTPUT(PASS);
}

/* Divide Zero Test
 * 
 * There should be a exception.
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: Exception
 * Files: IDT.h/c
 */
void divide_zero_test(){
	TEST_HEADER;

	int a = 0;
	a = 100 / a;
	printf("\nThis line should not occur.\n");

	TEST_OUTPUT(FAIL);
}

/* Easy Page Test
 * 
 * Check some paged addresses
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: Page, IDT
 * Files: page.h/c, IDT.h/c
 */
void page_easy_test(void) {
	TEST_HEADER;

	int a;

	printf("\n----Test for 0x400000 (first address of 4-8MB block)----\n");
	a = *((int *)0x400000);
	printf("0x400000 (first address of 4-8MB block) is paged.\n");

	printf("\n----Test for 0x7FFFFC (last address of 4-8MB block)----\n");
	a = *((int *)0x7FFFFC);
	printf("0x7FFFFC (last address of 4-8MB block) is paged.\n");

	printf("\n----Test for 0xB8000 (first address of video memory)----\n");
	a = *((int *)0xB8000);
	printf("0xB8000 (first address of video memory) is paged.\n");

	printf("\n----Test for 0x800000 (Not Present Page)----\n");
	a = *((int *)0x800000);
	printf("\nThis line should not occur.\n");

	TEST_OUTPUT(FAIL);
}

/* Page FAULT Test
 * 
 * Check some paged addresses
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: Page, IDT
 * Files: page.h/c, IDT.h/c
 */
void page_fault_test(void) {
	TEST_HEADER;

	int a;

	printf("\n----Test for 0x12345678 (Not Present Page)----\n");
	a = *((int *)0x12345678);
	printf("\nThis line should not occur.\n");

	TEST_OUTPUT(FAIL);
}

/* CP3.1 Hybrid Test
 * 
 * See codes for details.
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: IDT, keyboard, rtc, page
 * Files: IDT.h/c, Device/keyboard_handler.h/c, x86_desc.h/S, page.h/c
 */
void cp1_hybrid_test(void) {
	TEST_HEADER;

	int a;

	_PULSE(100000000);
	idt_test();

	_PULSE(500000000);
 	basic_keyboard_test();

	_PULSE(500000000);
 	rtc_test();
	clear_reset_cursor();

	_PULSE(100000000);
 	irq_syscall_test();

	_PULSE(500000000);
	printf("\n---- *((int *) 0xFFEE0080) / 0 ----\n");
	printf("Since 0xFFEE0080 is not paged, there should be a page fault\n");
	printf("exception before the divided by 0 exception. If it occurs a\n");
	printf("page fault exception, it works well. \n");
	a = 0;
	a = *((int *) 0xFFEE0080) / a;
	printf("\nThis line should not occur.\n");
	
	TEST_OUTPUT(FAIL);
}

/* Checkpoint 2 tests */

/* Terminal Elementary Test
 * 
 * Read from the terminal, and print what we have read.
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: Keyboard, terminal
 * Files: Device/keyboard_handler.h/c
 */
void terminal_easy_test() {
	TEST_HEADER;
	
	while (1) {
		char buf[128];
		int c = terminal_read(1, buf, 128);   // nbyte does not matter here
		cli();
		terminal_write(1, buf, c);
		sti();
	}

	TEST_OUTPUT(PASS);
}

/* Shell Length Test
 * 
 * Read from the terminal, and print the length.
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: Keyboard, terminal
 * Files: Device/keyboard_handler.h/c, terminal.h/c
 */
void shell_len_test() {
	TEST_HEADER;
	
	while (1) {
		char buf[128];
		int c = terminal_read(1, buf, 128);   // nbyte does not matter here
		cli();
		printf("We have %d char(s) in total, including the EOL.\n", c);
		sti();
	}

	TEST_OUTPUT(PASS);
}

/* Spam Output Test
 * 
 * Spam the output and test CTRL+L.
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: Keyboard, terminal
 * Files: Device/keyboard_handler.h/c, terminal.h/c
 */
void spam_output_test() {
	TEST_HEADER;

	int i, j;
	for (i = 0; i < 121; ++i) {
		printf("This is the test line #%d: ", i);
		for (j = 0; j < i; ++j) {
			printf("[%d]", i + j);
		}
		printf("\n");
		_PULSE(20000000);
	}

	TEST_OUTPUT(PASS);
}

/* RTC test
 * 
 * Test RTC.
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: rtc
 * Files: rtc.h/c
 */
void rtc_cp2_test() {
	TEST_HEADER;

	int32_t i, tmp, j;

	rtc_open(NULL);

	printf("Enabling rtc test");
	for (i = 0; i < 6; ++i) {
	    _PULSE(50000000);
		printf(".");
	}
	clear_reset_cursor();

	for (i = 1; i < 9; ++i) {
		tmp = 1 << i;
		rtc_write(1, &tmp, sizeof(tmp));
		for (j = 0; j < (4 << i); ++j) {
			printf("%d", j >> i);
			rtc_read(1, NULL, 0);
		}
		clear_reset_cursor();
	}

	TEST_OUTPUT(rtc_close(0) == 0);
}

/* List All Files Test
 * 
 * list all the file in the current directory
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: filesystem
 * Files: FileSystem.h/c
 */
// void list_all_files() {
// 	TEST_HEADER;
	
// 	int i;
// 	int32_t start = 0;
// 	for (i = 0; i < ((boot_block_t*)fsys_addr) -> num_dentry; ++i) {
// 		char buf[33];
// 		dir_read_cp2(&start, buf, 0);
// 		printf("file_name: ");
// 		printf("%s\n", buf);
// 	}

// 	TEST_OUTPUT(PASS);
// }

/* Read File by Name Test
 * 
 * read a file by its name
 * Inputs: None
 * Outputs: None
 * Side Effects: None
 * Coverage: filesystem
 * Files: FileSystem.h/c
 */
// void read_file_by_name() {
// 	TEST_HEADER;
	
// 	char filename[128];
// 	int i;
// 	int32_t start = 0;
// 	for (i = 0; i < ((boot_block_t*)fsys_addr) -> num_dentry; ++i) {
// 		char buf[33];
// 		dir_read_cp2(&start, buf, 0);
// 		printf("file #%d: %s\n", i, buf);
// 	}

// 	printf("\nPlease enter the name of the file you wanna lookup... (enter nothing to quit)\n");

// 	int c;
// 	while ((c = terminal_read(1, filename, 128))> 1) {
// 		filename[c - 1] = '\0';
// 		uint32_t offset = 0;
// 		uint32_t* offset_pointer = &offset;
// 		int32_t i_node = file_open_cp2((uint8_t*) filename);

// 		printf("%d\n", i_node);

// 		if (i_node > 0) {
// 			uint8_t buf[40000];
// 			int32_t num_byte = file_read_cp2((int32_t) i_node, offset_pointer, buf, 40000);

// 			uint32_t i;
// 			for(i = 0; i < num_byte; ++i){
// 				if (buf[i] != '\0') {
// 					putc(buf[i]);
// 				}
// 			}
// 			printf("\nFile name: %s | File length: %d.\n", filename, num_byte);
// 		} else {
// 			printf("Sorry, we cannot do %s.\n", filename);
// 		}
// 		printf("\nPlease enter the name of the file you wanna lookup... (enter nothing to quit)\n");
// 	}
	

// 	TEST_OUTPUT(PASS);
// }

/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
    test();
}
