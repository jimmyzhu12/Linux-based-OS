#include "cp5.h"
#include "FileSystem.h"
#include "SystemCall.h"
#include "x86_desc.h"
#include "page.h"
#include "lib.h"

#define VIDEO_PHY			0x80000
#define VIDEO_PHY_SHIFT 	12
/*
 * terminals_init
 *   DESCRIPTION: initialize the terminals
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: change the terminal_t struct
 */
void terminals_init()
{
    // initializing global vairables
    cur_terminal = 0;
    run_terminal = 0;
    // for initializing iteration
    int i, j;
    for (i = 0; i < NUMBER_OF_TERMINAL; i++)
    {
        terminals[i].id = i;
        terminals[i].booted = 0;
        terminals[i].video_mem_phys = (uint8_t*) (VIDEO_PHY + (i+1)*SIZE_4KB);
        terminals[i].cursor_x = 0;
        terminals[i].cursor_y = 0;
        terminals[i].num_process = 0;
        for (j = 0; j < MAX_PROCESS; j++)
        {
            terminals[i].pids[j] = -1;  // -1 for invalid
        }

        // welcome information
        welcome_printed[i] = 0;

        // initialize the video memory of each terminal
        clear_terminal_memory(terminals[i].video_mem_phys);
    }
    welcome_printed[0] = 1;
}

/*
 * terminal_switch
 *   DESCRIPTION: switch the terminal, not only to display
 *   INPUTS: new_terminal - id of the new terminal
 *   OUTPUTS: the desired terminal will be displayed
 *   RETURN VALUE: none
 *   SIDE EFFECTS: change paging
 */
void terminal_switch(uint32_t new_terminal)
{
    uint32_t previous_cur_terminal;
    previous_cur_terminal = cur_terminal;
    // I think no interrupts should be permitted during the process of switching terminal
    cli();
    // if current terminal, nothing need to do
    if (new_terminal == cur_terminal)
    {
        sti();
        return;
    }

    // make sure the map of VIDEO_PHY is VIDEO_PHY
    set_kernel_map_for_copying();

    // if terminal not running, start it.
    // if (terminals[new_terminal].booted == 0)
    // {
    //     save_terminal_contents(cur_terminal);
    //     // clear the screen and reset the cursor
    //     cur_terminal = new_terminal;
    //     clear_reset_cursor();
    //     run_terminal = new_terminal;
    //     after_setting_run_reset_kernel_video_map();
    //     terminals[new_terminal].booted = 1;
    //     execute((uint8_t*) "shell");   // ignore the following codes in this function
    //     sti();
    //     return;
    // }

    // save the current content on the screen into the current terminal physical video address
    save_terminal_contents(cur_terminal);
    // restore the desired terminal content
    restore_terminal_contents(new_terminal);


    // reset the video mappings of the processes. the process in cur_terminal and new_terminal will be reset
    int p, num_process_of_cur, num_process_of_new;
    num_process_of_cur = terminals[previous_cur_terminal].num_process;
    num_process_of_new = terminals[new_terminal].num_process;
    cur_terminal = new_terminal;
    // reset the kernel video address
    after_setting_run_reset_kernel_video_map();

    // reset the video mappings of the current terminal
    for (p = 0; p < num_process_of_cur; p++)
    {
        uint32_t process_id = terminals[previous_cur_terminal].pids[p];
        // process's physical memory. the processes in the current terminal will be mapped into current terminal's memroy
        uint32_t p_mem = VIDEO_PHY + SIZE_4KB * (previous_cur_terminal + 1);
        // reset the memory mapping
        reset_page(process_id, p_mem);
    }
    // reset the video mapping of the new terminal
    for (p = 0; p < num_process_of_new; p++)
    {
        uint32_t process_id = terminals[new_terminal].pids[p];
        // process's physical memory. the processes in the new terminal will be mapped into the displaying video memory
        uint32_t p_mem = VIDEO_PHY;
        // reset the memory mapping
        reset_page(process_id, p_mem);
    }

    sti();
    return;
}

/*
 * save_terminal_contents
 *   DESCRIPTION: save the contents of the (current) screen into the assigned temrinal memory
 *   INPUTS: terminal_id - the assigned terminal
 *   OUTPUTS: the current contents on the screen will be stored into the terminal physical video memory address
 *   RETURN VALUE: none
 *   SIDE EFFECTS: change physical video memory of the assigned terminal
 */
void save_terminal_contents(uint32_t terminal_id)
{
    // copy the memory of the screen into the assignd terminl physical memory address
    memcpy((uint8_t*) terminals[terminal_id].video_mem_phys, (uint8_t*) VIDEO_PHY, SCREEN_SIZE);
}

/*
 * restore_terminal_contents
 *   DESCRIPTION: restore the contents of assigned temrinal memory into the (current) screen physical memory address
 *   INPUTS: terminal_id - the assigned terminal
 *   OUTPUTS: the desired terminal will be load onto the screen
 *   RETURN VALUE: none
 *   SIDE EFFECTS: change physical video memory
 */
void restore_terminal_contents(uint32_t terminal_id)
{
    update_cursor(terminals[terminal_id].cursor_x, terminals[terminal_id].cursor_y);
    // copy the memory of the screen into the assignd terminl physical memory address
    memcpy((uint8_t*) VIDEO_PHY, (uint8_t*) terminals[terminal_id].video_mem_phys, SCREEN_SIZE);
}

/*
 * push_process
 *   DESCRIPTION: push a running process into a terminal
 *   INPUTS: terminal_id - the assigned terminal
 *           process_id - the process id that is going to be pushed
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: change the assigned terminal structure
 */
void push_process(uint32_t terminal_id, uint32_t process_id)
{
    terminals[terminal_id].pids[terminals[terminal_id].num_process] = process_id;
    terminals[terminal_id].num_process += 1;
}

/*
 * pop_process
 *   DESCRIPTION: pop a running process into a terminal
 *   INPUTS: terminal_id - the assigned terminal
 *   OUTPUTS: none
 *   RETURN VALUE: the poped process id
 *   SIDE EFFECTS: change the assigned terminal structure
 */
uint32_t pop_process(uint32_t terminal_id)
{
    int32_t return_value;
    return_value = terminals[terminal_id].pids[terminals[terminal_id].num_process - 1];
    terminals[terminal_id].pids[terminals[terminal_id].num_process - 1] = -1;
    terminals[terminal_id].num_process -= 1;
    return return_value;
}

/*
 * reset_page
 *   DESCRIPTION: reset the 4KB paging from v_mem to p_mem
 *   INPUTS: v_mem - the virtual memory that is going to be reset
 *           p_mem - the physical memory that is going to be set as the memory of v_mem
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: the 4KB page of v_mem will be reset to physical p_mem
 */
void reset_page(uint32_t process_id, uint32_t p_mem)
{
	pte_vid[process_id].p_base = ((uint32_t)p_mem >> SHIFT_4KB);
	flush_TLBs();
}

/*
 * set_kernel_map_for_copying()
 *   DESCRIPTION: reset the 4KB paging from DPL = 0 video memory to the place it should be mapped to
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: the 4KB page of the kernel video memory will be reset
 */
void set_kernel_map_for_copying()
{
    pte[VIDEO_PHY >> VIDEO_PHY_SHIFT].p_base = ((uint32_t)VIDEO_PHY >> SHIFT_4KB);
    flush_TLBs();
}

/*
 * reset kernel_video_map()
 *   DESCRIPTION: reset the 4KB paging from DPL = 0 video memory to the place it should be mapped to
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: the 4KB page of the kernel video memory will be reset
 */
void reset_kernel_video_map(uint32_t new_terminal)
{
    if (new_terminal == run_terminal)
    {
        pte[VIDEO_PHY >> VIDEO_PHY_SHIFT].p_base = ((uint32_t)VIDEO_PHY >> SHIFT_4KB);
    }
    else
    {
        pte[VIDEO_PHY >> VIDEO_PHY_SHIFT].p_base = ((uint32_t)(VIDEO_PHY + SIZE_4KB * (run_terminal + 1))>> SHIFT_4KB);
    }
    flush_TLBs();
}

/*
 * reset after_setting_run_reset_kernel_video_map
 *   DESCRIPTION: reset the 4KB paging from DPL = 0 video memory to the place it should be mapped to. should be called everytime when the run_terminal is set
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: the 4KB page of the kernel video memory will be reset
 */
void after_setting_run_reset_kernel_video_map()
{
    if (cur_terminal == run_terminal)
    {
        pte[VIDEO_PHY >> VIDEO_PHY_SHIFT].p_base = ((uint32_t)VIDEO_PHY >> SHIFT_4KB);
    }
    else
    {
        pte[VIDEO_PHY >> VIDEO_PHY_SHIFT].p_base = ((uint32_t)(VIDEO_PHY + SIZE_4KB * (run_terminal + 1))>> SHIFT_4KB);
    }
    flush_TLBs();
}
