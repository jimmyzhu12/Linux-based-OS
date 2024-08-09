#ifndef _MULTI_TERMINAL_SCHEDULER_H
#define _MULTI_TERMINAL_SCHEDULER_H

#include "SystemCall.h"

// moved to lib.h

// #define NUMBER_OF_TERMINAL 3
// // struct for terminal
// typedef struct terminal_t{
//     uint32_t id;   // terminal id. 0, 1 or 2.
//     uint32_t booted;        // 1 if the terminal has already started before
//     uint8_t* video_mem_phys;    // physical memory for the video content in this terminal
//     uint32_t cursor_x;      // x coordinate of the cursor
//     uint32_t cursor_y;      // y coordinate of the cursor
//     uint32_t num_process;           // the number of processes running in this terminal
//     uint32_t pids[MAX_PROCESS];     // the process ids in this terminal. -1 for not running
// } terminal_t;

// // the terminals
// terminal_t terminals[NUMBER_OF_TERMINAL];
// int welcome_printed[NUMBER_OF_TERMINAL];

// moved to lib.h end

void terminals_init();
void terminal_switch(uint32_t new_terminal);
void save_terminal_contents(uint32_t terminal_id);
void restore_terminal_contents(uint32_t terminal_id);

// for easier terminal usage
void push_process(uint32_t terminal_id, uint32_t process_id);
uint32_t pop_process(uint32_t terminal_id);

// for page resetting
void set_kernel_map_for_copying();
void reset_kernel_video_map(uint32_t new_terminal);
void after_setting_run_reset_kernel_video_map();
void reset_page(uint32_t process_id, uint32_t p_mem);

#endif /*_MULTI_TERMINAL_SCHEDULER_H */
