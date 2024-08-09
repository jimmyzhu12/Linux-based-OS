
#include "FileSystem.h"
#include "SystemCall.h"
#include "x86_desc.h"
#include "page.h"
#include "lib.h"
#include "cp5.h"
#include "Device/keyboard_handler.h"

#define VIDEO_PHY			0x80000 /* copy (and modify) from lib.c */
#define RAND_MAX			0x7FFFFFFF

/*
 * abort_read
 *   DESCRIPTION: should not do this read operation
 *   INPUTS: none
 *   OUTPUTS:
 *   RETURN VALUE: -1 
 *   SIDE EFFECTS: none
 */
int32_t abort_read(int32_t fd, void* buf, int32_t nbytes) {
    return FAIL;
}
/*
 * abort_write
 *   DESCRIPTION: should not do this write operation
 *   INPUTS: none
 *   OUTPUTS:
 *   RETURN VALUE: -1 
 *   SIDE EFFECTS: none
 */
int32_t abort_write(int32_t fd, const void* buf, int32_t nbytes) {
    return FAIL;
}

 
/*
 *   fop_t_init
 *   DESCRIPTION: initialize five file operations table: stdin, stdout, rtc, dir, file
 *   INPUTS: none
 *   OUTPUTS:none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void fop_t_init(){
    stdin_op.open   = terminal_open;
    stdin_op.close  = terminal_close;
    stdin_op.read   = terminal_read;
    stdin_op.write  = abort_write;

    stdout_op.open  = terminal_open;
    stdout_op.close = terminal_close;
    stdout_op.read  = abort_read;
    stdout_op.write = terminal_write;

    rtc_op.open   = rtc_open;
    rtc_op.close  = rtc_close;
    rtc_op.read   = rtc_read;
    rtc_op.write  = rtc_write;

    dir_op.open   = dir_open;
    dir_op.close  = dir_close;
    dir_op.read   = dir_read;
    dir_op.write  = dir_write;

    file_op.open  = file_open;
    file_op.close = file_close;
    file_op.read  = file_read;
    file_op.write = file_write;
}

/*
 * open
 *   DESCRIPTION: find the directory entry corresponding to the named file, allocate an unused file descriptor
 *                set up any data necessary to handle the given type of file 
 *                (directory, RTC device, or regular file).            
 *   INPUTS: filename -- target file to open
 *   OUTPUTS: none
 *   RETURN VALUE: "fd" on success; -1 when named file does not exist or no descriptors are free
 *   SIDE EFFECTS: none
 */
int32_t open (const uint8_t* filename){
    sti();
    // sanity check
	if (filename == NULL) {
		return -1;
	}

	// check available fd 
	pcb_t* cur_pcb_p = get_pcb_ptr(pid);
	int32_t fd_index = 2; /* 0 and 1 are stdin/stdout */
	dentry_t cur_dentry;

	/* define MAX_FILE_OPENED as 8. Magic Number */
	while (fd_index < FD_NUM &&							\
	       cur_pcb_p->fds[fd_index].flag != fd_idle)
	{
		++fd_index;
	}
	if (fd_index == FD_NUM) {
		return -1;
	}

    // check the existence of the file
	if (read_dentry_by_name(filename, &cur_dentry) == -1) {
		return -1;
	}

    // allocate a new fd, fill all fields except flag
	if (filename[0] == '.' && filename[1] == '\0') {
		/* the file is "." */
		cur_pcb_p->fds[fd_index].fop = &dir_op;
	} else if (filename[0] == 'r' && filename[1] == 't' &&			\
	           filename[2] == 'c' && filename[3] == '\0')
	{
		/* the file is "rtc" */
		cur_pcb_p->fds[fd_index].fop = &rtc_op;
	} else {
		cur_pcb_p->fds[fd_index].fop = &file_op;
	}
	cur_pcb_p->fds[fd_index].fseek = 0;
	cur_pcb_p->fds[fd_index].idx_inode = cur_dentry.inode;

    // call the corresponding open function
	if (cur_pcb_p->fds[fd_index].fop->open(filename) < 0) {
		// fail
		// cur_pcb_p->fds[fd_index].flag = fd_error;
		return -1;
	}
    cur_pcb_p->fds[fd_index].flag = fd_busy;
    return fd_index;
}

/*
 * close
 *   DESCRIPTION: close a file           
 *   INPUTS: fd -- file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
int32_t close (int32_t fd){
    sti();
	// sanity check
	if (fd < 0 || fd > FD_NUM) {
		return -1;
	}
	pcb_t* cur_pcb_p = get_pcb_ptr(pid);
	if (cur_pcb_p == NULL) {
		return -1;
	}
	if (cur_pcb_p->fds[fd].flag == fd_idle) {
		return -1;
	}

	// call the corresponding close function
	if (cur_pcb_p->fds[fd].fop->close(fd) < 0) {
		// fail
		// cur_pcb_p->fds[fd].flag = fd_error;
		return -1;
	}
	cur_pcb_p->fds[fd].fop = NULL;
	cur_pcb_p->fds[fd].idx_inode = -1;
	cur_pcb_p->fds[fd].fseek = 0;
	cur_pcb_p->fds[fd].flag = fd_idle;
	return 0;
}

/*
 * read
 *   DESCRIPTION: read a file        
 *   INPUTS: fd -- file descriptor
 *           buf - where the data read to
 *           nbytes - number of bytes to read
 *   OUTPUTS: none
 *   RETURN VALUE: bytes read on success, -1 on failure, 0 when initial file position is at or beyond the end of file 
 *   SIDE EFFECTS: none
 */

int32_t read (int32_t fd, void* buf, int32_t nbytes){
    sti();
    // ATTENTION: synchronization issue!

    //  keyboard: read should return data from one line that has been terminated by pressing Enter, 
    //  or as much as fits in the buffer from one such line. 
    //  The line returned should include the line feed character.

    // file: data should be read to the end of the file or the end of the buffer provided, 
    // whichever occurs sooner

    // directory:  only the filename should be provided (as much as fits, or all 32 bytes)
    // subsequent reads should read from successive directory entries until the last is reached, 
    // at which point read should repeatedly return 0

    // RTC: return 0, but only after an interrupt has occurred 
    // (set a flag and wait until the interrupt handler clears it, then return 0)

	// sanity check
	if (fd < 0 || fd > FD_NUM) {
		return -1;
	}
	pcb_t* cur_pcb_p = get_pcb_ptr(pid);
	if (cur_pcb_p == NULL) {                        // not because this it returns -1
        
		return -1;
	}
	if (cur_pcb_p->fds[fd].flag != fd_busy) {       // not because this it returns - 
        
		return -1;
	}

	// call the corresponding read function
	return cur_pcb_p->fds[fd].fop->read(fd, buf, nbytes);
}

/*
 * write
 *   DESCRIPTION: writes data to the terminal or to a device (RTC)
 *   INPUTS: fd -- file descriptor
 *           buf - stores the data to be written
 *           nbytes - number of bytes to write
 *   OUTPUTS: none
 *   RETURN VALUE: bytes written on success, -1 on failure (as well as writing to file/dir)
 *   SIDE EFFECTS: none
 */
int32_t write (int32_t fd, const void* buf, int32_t nbytes){
    // sanity check
    sti();
	if (fd < 0 || fd > FD_NUM) {
		return -1;
	}
	pcb_t* cur_pcb_p = get_pcb_ptr(pid);
	if (cur_pcb_p == NULL) {       
        
		return -1;
	}
	if (cur_pcb_p->fds[fd].flag != fd_busy) {    
        
		return -1;
	}

	// call the corresponding write function
    return  cur_pcb_p->fds[fd].fop->write(fd, buf, nbytes);
    // ATTENTION: synchronization issue!
    // rtc: accept only a 4-byte integer specifying the interrupt rate in Hz, and should set the rate of periodic interrupts accordingly.
}

/*
 * execute
 *   DESCRIPTION: envoke a user program from kernel
 *   INPUTS: command - program name and args
 *   OUTPUTS:
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS:
 */
int32_t execute (const uint8_t* command){

    uint8_t fname[33];    // fname[32] must be '\0'
    uint8_t args[ARG_LEN];    // 128 is the line buffer length
    dentry_t new_dentry;

    uint8_t file_header[HEADER_LEN];
    uint8_t* program_addr;      // 
    uint8_t is_shell = 0;       // whether executing shell
    uint32_t length_of_fname;

    // cp5: multi-terminals, deleted after combining scheduler
    // run_terminal = cur_terminal;
    // after_setting_run_reset_kernel_video_map();

    // sanity check
    if (command == NULL) return FAIL;

    // 1.parse args
    if( _parse_arg(command, fname, args) == FAIL )              return FAIL;

    // 2.check file validity : 1) file name existence 2) check executable file
    if(read_dentry_by_name(fname, &new_dentry) == FAIL)          return FAIL;
    if(read_data(new_dentry.inode, 0, file_header, HEADER_LEN) == FAIL) return FAIL;
	// These magic numbers are used to verify whether the file is executible.
    if(file_header[0] != 0x7f || file_header[1] != 0x45 || 
       file_header[2] != 0x4c || file_header[3] != 0x46){
       printf(" NOT A EXECUTABLE FILE!");
       return FAIL;
    }
    

    // 3.find the pid for this process
    int i;
    uint32_t last_pid = pid;
    for (i=0; i<MAX_PROCESS; i++){
        if(process[i] == 0){
            process[i] = 1;
            pid = i;
            break;
        }
    }
    if(i == MAX_PROCESS){
        printf(" MAX_PROCESS has been reached, calm down you crazy man!");
        return FAIL;
    }
    parent_pid = last_pid;

    // 4.set up paging to user program
    _set_user_paging(pid);

    // 5.load file (user program) into memory
    program_addr = (uint8_t*)0x8048000;             // where the programe should start
    read_data(new_dentry.inode, 0, program_addr, get_size(new_dentry.inode));

    // 6.set PCB and fd  {TODO: process the pid!!!!!!!!!!!}
    pcb_t* current_pcb = get_pcb_ptr(pid);
    // printf("%x\n", (int32_t)current_pcb);
        // user program entry point 
    current_pcb->eip = (uint32_t*)(program_addr + 24);  
    current_pcb->parent_pid = parent_pid;      
    current_pcb->pid = pid;  
        // fill the args
    strncpy((int8_t*)current_pcb->args, (int8_t*)args, ARG_LEN);
        // initialize the fd
    for(i=0; i<FD_NUM ; i++){
        switch(i){
            case 0: // stdin
                current_pcb->fds[i].flag = fd_busy;
                current_pcb->fds[i].fop  = &stdin_op;
                current_pcb->fds[i].idx_inode = FAIL;
                current_pcb->fds[i].fseek = FAIL;
                break;
            case 1: // stdout
                current_pcb->fds[i].flag = fd_busy;
                current_pcb->fds[i].fop  = &stdout_op;
                current_pcb->fds[i].idx_inode = FAIL;
                current_pcb->fds[i].fseek = FAIL;
                break;
            default: // others
                current_pcb->fds[i].flag = fd_idle;
                break;
        }
    }

    // 6.5. cp5: multi_terminals:
    push_process(run_terminal, pid);
    // not sure if useful 
    current_pcb->running_terminal = cur_terminal;
    
    length_of_fname = strlen_xjz(fname);
    if ((length_of_fname == 5) & (fname[0] == 's') & (fname[1] == 'h') & (fname[2] == 'e') & (fname[3] == 'l') & (fname[4] == 'l'))
    {
        is_shell = 1;
    }


    // 7.context swith 
        // set TSS
    tss.ss0     = KERNEL_DS;
    tss.esp0    = KERNEL_END - PCB_SIZE * pid - 4;
        // set stack (the same order on the stack)
    uint32_t EIP = *current_pcb->eip;
    uint32_t CS  = USER_CS;
    uint32_t ESP = _USER_ESP_;
    uint32_t SS  = USER_DS;
        // save stack information
    register uint32_t saved_esp asm("esp");
    register uint32_t saved_ebp asm("ebp");
    current_pcb->esp = saved_esp;
    current_pcb->ebp = saved_ebp;
        // switch!
        // deleted cli in asm 
    asm volatile(
        "cli;"
        "movw  %%dx, %%ds;"
        "pushl %%edx;"
        "pushl %%ecx;"
        "pushfl  ;"
        "pushl %%ebx;"
        "pushl %%eax;"
        "IRET;"
        : 
        : "a"(EIP), "b"(CS), "c"(ESP), "d"(SS)
        : "memory"
    );

    int32_t val;
    asm volatile(
        "final_return:"
        "movl  %%eax, %0;"
        : "=r"(val)
    );
    return val;
}


/*
 * halt
 *   DESCRIPTION: terminates a process
 *   INPUTS: status - used to determine whether the halt is due to exception or not. 1 for exception, 0 for not.
 *   OUTPUTS:
 *   RETURN VALUE: 0 for success, 1 if halt by exception
 *   SIDE EFFECTS: switch context; close relevant FD
 */
 // The flow of this function is discussed in MP 3.3 Review Session
int32_t halt (uint8_t status){
    // initialize variables
    pcb_t* current_pcb;
    pcb_t* parent_pcb;
    int32_t return_value;
    int32_t i;      // for iterations
    // uint32_t running_pid;   // used for adding control + c
    // uint32_t old_pid;   // cp5

    // set up return value
    if (status == 1)
    {
        return_value = EXCEPTION_RETURN;
    }
    else
    {
        return_value = 0;
    }

    cli();
    // close all processes?
    // cp5: multi-terminals
    pop_process(run_terminal);
    // running_pid = pop_process(run_terminal);
    // pid = running_pid;
    // set currently-active-process to non-active
    process[pid] = 0;

    
    // reset the running terminal
    // run_terminal = cur_terminal; wrong!

    // get current pcb
    current_pcb = get_pcb_ptr(pid);

    // cp5: init the keyboard enter buffer.
    init_enter();

    // // check if main shell
    if (current_pcb->parent_pid == -1)
    {
        // close files that are relavant: stdin, stdout and 6 files (if open)
        current_pcb->fds[0].flag = fd_idle;
        current_pcb->fds[1].flag = fd_idle;
        for (i = 2; i < 8; i++)
        {
            if (current_pcb->fds[i].flag == fd_busy)
            {
                close(i);
            }
        }
        // sti();
        pid = -1;
        execute((uint8_t*)"shell");
    }
    else{
        // restore parent data
        // get parent process
        parent_pcb = get_pcb_ptr(current_pcb->parent_pid);

        // set TSS for parent
        tss.ss0 = KERNEL_DS;
        tss.esp0 = KERNEL_END - PCB_SIZE * (parent_pcb->pid) - 4;    // -4 because base of stack

        // unmap paging for current process?

        // map parent's paging
        _set_user_paging(parent_pcb->pid);

        // close files that are relavant: stdin, stdout and 6 files (if open)
        current_pcb->fds[0].flag = fd_idle;
        current_pcb->fds[1].flag = fd_idle;
        for (i = 2; i < 8; i++)
        {
            if (current_pcb->fds[i].flag == fd_busy)
            {
                close(i);
            }
        }

        process[parent_pcb->pid] = 1;
        pid = parent_pcb->pid;
        // cp5: multi_terminals
        // run_terminal = parent_pcb->running_terminal;
        sti();
    }

    // halt return
    asm volatile(
        "movl   %2, %%esp;"
        "movl   %1, %%ebp;"
        "movl   %0, %%eax;"
        "leave;"
        "ret;"
        :   // no outputs
        :   "r"(return_value), "r"(current_pcb->ebp), "r"(current_pcb->esp)
        :   "eax", "esp", "ebp"
    );

    // actually won't be here
    return 0;
  
}




//---------------------------CP4 Part-------------------------------
/*
 * getargs
 *   DESCRIPTION: reads the program's command line arguments into a user-level buffer
 *   INPUTS: buf, the buffer to be copied into about the arguments
 *           nbytes, the number of bytes to be copied into
 *   OUTPUTS: None
 *   RETURN VALUE: 0 for success, -1 if fails
 *   SIDE EFFECTS: write into an existing buffer
 */
int32_t getargs (uint8_t* buf, int32_t nbytes){
    pcb_t* current_pcb;
    current_pcb = get_pcb_ptr(pid);                         // get the current PCB

    if (buf == NULL || nbytes == 0) {                       // if the buffer is null, or there is no length to be copied
        return -1; 
    }
    

    char * argument = (char *) current_pcb->args;

    if (argument == NULL || argument[0] == '\0') {                                  // if there is no arguments, return -1
        return -1;
    }
    strncpy((char*) buf, argument, nbytes);                                           // dest src length
    return 0;                                               // return 0 on success
}

/*
 * vidmap
 *   DESCRIPTION: maps the text-mode video memory into user space at a pre-set virtual address
 *   INPUTS: screen_start, a pointer of a pointer to screen start
 *   OUTPUTS: None
 *   RETURN VALUE: 0 for success, -1 if fails
 *   SIDE EFFECTS:  write into user space
 */
int32_t vidmap (uint8_t** screen_start){
	/* sanity check */
	if (screen_start == NULL) {
		return -1;
	}
	if ((uint32_t)screen_start < USER_ST || (uint32_t)screen_start > _USER_ESP_) {
		return -1;
	}
	pde[VIDEO_ST >> SHIFT_4MB]._4KB.ps  = 0;			/* page size(0 -> 4KB)	*/
	pde[VIDEO_ST >> SHIFT_4MB]._4KB.p   = 1;			/* present!				*/
	pde[VIDEO_ST >> SHIFT_4MB]._4KB.u_s = 1;			/* visible to user		*/
	pde[VIDEO_ST >> SHIFT_4MB]._4KB.r_w = 1;			/* always r/w avaliable	*/
						/* point to the page-table entry	*/
	pde[VIDEO_ST >> SHIFT_4MB]._4KB.pt_base = (uint32_t)pte_vid >> SHIFT_4KB;
	pte_vid[pid].p = 1;
	pte_vid[pid].p_base = ((uint32_t)VIDEO_PHY >> SHIFT_4KB);// + pid;
	flush_TLBs();
	*screen_start = (uint8_t*)(VIDEO_ST + pid * SIZE_4KB);
// #define print(hex) printf("%x\n", (uint32_t)(hex));
// // 	printf("%x %x\n", VIDEO_ST >> SHIFT_4MB, (uint32_t)pte_vid >> SHIFT_4KB);
// // 	printf("%x %x \n", (uint32_t)(pte_vid[pid].p_base), (VIDEO_ST + pid * SIZE_4KB));
// 	// print(VIDEO_ST >> SHIFT_4MB);
// 	// print(pde);
// 	// print(pte);
// 	// print(pte_vid);
// 	// print((uint32_t)(pte_vid[pid].p_base) << SHIFT_4KB);
// 	// print(VIDEO_ST + pid * SIZE_4KB);
// 	// print(*(int8_t*)(VIDEO_ST + pid * SIZE_4KB + 0x60));
// 	// print(*(int8_t*)(VIDEO_ST + (pid + 1) * SIZE_4KB - 1));
// #undef print

    return 0;// (int32_t)(*screen_start);
}
//------------------------auxilliary functions----------------------------

/*
 * get_pcb_ptr
 *   DESCRIPTION: find the PCB pointer given certain pid
 *   INPUTS: pid -- process id
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: modify pde[32], flush TLB
 */
pcb_t* get_pcb_ptr(uint32_t pid){
    return (pcb_t*) (KERNEL_END - PCB_SIZE*(pid+1));
}   
/*
 * _set_user_paging
 *   DESCRIPTION: use 4MB paging for user program (executable file).
 *   INPUTS: pid -- process id
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: modify pde[32], flush TLB
 */
void _set_user_paging(uint32_t pid){

    // virtual memory for user program starts from 128MB, 128/4=32
    if (pde[32]._4MB.p == 0){
        pde[32]._4MB.p   = 1;   // present
        pde[32]._4MB.r_w = 1;   // R/W enable
        pde[32]._4MB.u_s = 1;   // for user code
        pde[32]._4MB.pwt = 0;   
        pde[32]._4MB.pcd = 0;
        pde[32]._4MB.a   = 0;   // TODO: check, NOT SURE!
        pde[32]._4MB.d   = 0;   // dirty bit should be 0 at first
        pde[32]._4MB.ps  = 1;   // for 4MB paging
        pde[32]._4MB.g   = 0;   // global page 0
        pde[32]._4MB.avail = 0;
        pde[32]._4MB.pat = 0;
        pde[32]._4MB.rev = 0; 
    }
    // set physical address
    pde[32]._4MB.p_base = pid + 2; // 8MB or 12MB

    flush_TLBs();
}

/* _parse_arg
 *  DESCIPTION: parse the command and get filename & arguments
 *  INPUTS:command  --  pointer to the input command
 *         fname    --  pointer to the parsed file_name
 *         args     --  pointer to the parsed args
 *  OUTPUTS: filled fname and args 
 *  RETURN VALUE: 0 on success, -1 on failure
 *  SIDE EFFECTs: none
 */
int32_t _parse_arg(const uint8_t* command, uint8_t* fname, uint8_t* args){
    uint32_t cmd_len = strlen((int8_t*) command);
    int32_t cmd_cursor = 0;
    int32_t fname_cursor = 0;
    int32_t args_cursor = 0;

    //get the fname
    while(command[cmd_cursor]==' ') {cmd_cursor++;}
    while(cmd_cursor < cmd_len && command[cmd_cursor] != ' '){
        if(fname_cursor >= FNAME_LEN) {return FAIL;}
        fname[fname_cursor] = command[cmd_cursor];
        fname_cursor ++;
        cmd_cursor ++;
    }
    fname[fname_cursor] = '\0';

    //get the args
    while(command[cmd_cursor]==' ') {cmd_cursor++;}
    while(cmd_cursor < cmd_len){
        if(args_cursor >= ARG_LEN) {return FAIL;}
        args[args_cursor] = command[cmd_cursor];
        args_cursor ++;
        cmd_cursor ++;
    }
    args[args_cursor] = '\0';

    return 0;
}

//---------------------------NOT USED YET--------------------------------
int32_t set_handler (int32_t signum, void* handler_address){
    return FAIL;
}
int32_t sigreturn (void){
    return FAIL;
}

static uint32_t seed = 56;
const uint32_t multiplier = 16807; // 7^5
const uint32_t quotient = 127773;
const uint32_t remainder = 2836; 

/* rand
 *  DESCIPTION: generate a random number usign seed (t = 7^5 * t (mod 2^31 -1))
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: random number
 *  SIDE EFFECTs: update the seed
 */
int32_t rand(void){

    int32_t t, quot, rem;
    quot = seed / quotient;
    rem = seed % quotient; 
    t = multiplier * rem - remainder * quot; 

    // update seed
    seed = t;

    return ((seed - 1) % (uint32_t) RAND_MAX);

}
/* set_seed
 *  DESCIPTION: set the seed for rand()
 *  INPUTS: val -- the seed to be set
 *  OUTPUTS: none
 *  RETURN VALUE: 0
 *  SIDE EFFECTs: none
 */
int32_t set_seed(uint32_t val){
    if (val == 0 || val == RAND_MAX){
        seed = 56;                    // choose a interesting value
    }
    else{
        seed = val;
    }
    return 0;
}

