#include "Scheduler.h"
#include "cp5.h"
#include "SystemCall.h"
#include "x86_desc.h"
#include "page.h"
#include "i8259.h"
#include "IDT_linkage.h"

#define DIVISOR_SHIFT	8
#define GUI_REDRAW		4

#define VIDEO_PHY		0x80000

extern void gui_render();

 /*
 * pic_init
 *   DESCRIPTION: initialize the PIT as timer chip for multi-task scheduler
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */

void pit_init(){
  // set mode
  outb(MODE, MODE_REG);
  // set frequency
  outb(DIVISOR & 0xFF, CH0_D_PORT);
  outb((DIVISOR & 0xFF)>>DIVISOR_SHIFT, CH0_D_PORT);
  // enable interrupt
  counter = 0;
  recursive_halt_counter_sche = 0;
  enable_irq((uint32_t) PIT_IRQNUM);
  return;
}

/* 
 * pic_init
 *   DESCRIPTION: initialize the pit as timer chip for multi-task scheduler system
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS:  initialize the pit, which will raise highest INT to PIC at period within 10ms - 50ms
 */
void pit_handler(){
	cli();
	//counter += 1;
	static int gui_counter = 1;
	if (gui_counter == GUI_REDRAW) {
		gui_render();
		gui_counter = 1;
	} else {
		++gui_counter;
	}
	send_eoi(PIT_IRQNUM);
	scheduler();
	sti();
	// printf_original("?");
	return;
}

/*
 * scheduler
 *   DESCRIPTION: switch the running terminals
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void scheduler(){

	if (halt_terminal == run_terminal){
		
		halt_terminal = -1;
		halt(0);
	}

	recursive_halt_counter_sche = (recursive_halt_counter_sche + 1) % 2;
	if (recursive_halt_counter_sche == 1)
	{
		if (recursive_halt_terminal == run_terminal){
			if (recursive_halt_counter > 1)
			{
				recursive_halt_counter -= 1;
				halt(0);
			}
			else if (recursive_halt_counter == 1)
			{
				recursive_halt_terminal = -1;
				recursive_halt_counter = -1;
				halt(0);
			}
		}
	}


	// record the running terminal before scheduling
	uint32_t from_terminal = run_terminal;

	run_terminal = (run_terminal+1) % NUMBER_OF_TERMINAL;
	after_setting_run_reset_kernel_video_map();	// TLB will be flushed at the end of this function

	pcb_t* old_pcb;
	pcb_t* new_pcb;
	old_pcb = get_pcb_ptr(terminals[from_terminal].pids[terminals[from_terminal].num_process - 1]);
	asm volatile (
		"movl %%ebp, %%eax;"
		"movl %%esp, %%ebx;"
		: "=a"(old_pcb->ebp_sch), "=b"(old_pcb->esp_sch)
		: /* no inputs */
	);
	// printf_original("\n%d, %x, %x\n", terminals[from_terminal].pids[terminals[from_terminal].num_process - 1], old_pcb->ebp_sch, old_pcb->esp_sch);
	// check if the terminal has been booted. If not, boot.
	if (terminals[run_terminal].booted == 0){
		terminals[run_terminal].booted = 1;
		pid = -1;
        printf("Welcome to terminal: %d \n", run_terminal + 1);
		execute((uint8_t*)"shell");
		return;
	}
	else{
		terminal_t new_terminal;
		uint32_t new_pid;
		new_terminal = terminals[run_terminal];
		new_pid = new_terminal.pids[new_terminal.num_process-1];

		// process switch
			//change TSS
		tss.ss0 = KERNEL_DS;
	    tss.esp0 = KERNEL_END - PCB_SIZE * new_pid - 4;
			// page setting
				// user paging
		pde[32]._4MB.p_base = new_pid + 2; 
				// video map : user and kernel
		uint32_t num_process_of_run = terminals[run_terminal].num_process;
		int p;
		if (run_terminal == cur_terminal){	// set the running process's page 
			for (p = 0; p < num_process_of_run; p++){
				// uint32_t process_id = terminals[run_terminal].pids[p];
				// reset the memory mapping
				reset_page(terminals[run_terminal].pids[p], VIDEO_PHY);
			}
		}
		else{	// set the running process's page
			for (p = 0; p < num_process_of_run; p++){
				// reset the memory mapping
				reset_page(terminals[run_terminal].pids[p], VIDEO_PHY + SIZE_4KB * (run_terminal + 1));
			}
		}
		

		// update the pid
		pid = new_pid;
		// stack change
		new_pcb = get_pcb_ptr(new_pid);
		// process jump
		
		// printf_original("%d, %x, %x\n", new_pid, new_pcb->ebp_sch, new_pcb->esp_sch);
		scheduler_switch(new_pcb->ebp_sch, new_pcb->esp_sch);
	}
}





