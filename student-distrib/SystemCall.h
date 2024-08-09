#ifndef _SYSTEMCALL_H
#define _SYSTEMCALL_H

#include "types.h"
#include "terminal.h"
#include "rtc.h"
#include "FileSystem.h"   

#define FNAME_LEN			32
#define HEADER_LEN			40
// moved to lib.h
// #define MAX_PROCESS			6
#define ARG_LEN				128
#define FD_NUM				8        
#define EXCEPTION_RETURN    256
#define FAIL				-1

#define PCB_SIZE			0x2000			//8KB

uint32_t process[MAX_PROCESS]; // for cp3/4, we only support two process
uint32_t pid;                // ranging from 0 to 1 for cp3/4
uint32_t parent_pid;

enum fd_status {
	fd_idle = 0,
	fd_busy = 1,
    fd_error = 2
};

typedef struct fop_t{
    int32_t (*open)(const uint8_t* fname);
    int32_t (*close)(int32_t fd);
    int32_t (*read)(int32_t fd, void* buf, int32_t nbytes);
    int32_t (*write)(int32_t fd, const void* buf, int32_t nbytes);
} fop_t;

fop_t stdin_op,stdout_op,rtc_op,dir_op,file_op;

typedef struct fd_t{
    fop_t* fop;               // pointer to the file operation jump table
    uint32_t idx_inode;       // inode number of the file
    uint32_t fseek;           // offset of the curser in that file ; idx for dir
    uint32_t flag;            // 1 for in-use, 0 for not
} fd_t;

typedef struct pcb_t{
    uint32_t pid;
    uint32_t parent_pid;

    fd_t fds[FD_NUM];              // support up to 8 open files
    uint8_t  args[ARG_LEN];   // argument for this process after parse

    uint32_t esp;             // initial esp
    uint32_t ebp;             // initial ebp

    uint32_t esp_sch;
    uint32_t ebp_sch;

    uint32_t* eip;             // for user program that will be called by this process

    // cp5: multi-terminals. not sure if useful
    uint32_t running_terminal;  // the terminal which this pid is running
} pcb_t;


void fop_t_init();


int32_t halt (uint8_t status);
//
int32_t execute (const uint8_t* command);
int32_t read (int32_t fd, void* buf, int32_t nbytes);
int32_t write (int32_t fd, const void* buf, int32_t nbytes);
int32_t open (const uint8_t* filename);
int32_t close (int32_t fd);
int32_t getargs (uint8_t* buf, int32_t nbytes);
int32_t vidmap (uint8_t** screen_start);
// 
int32_t set_handler (int32_t signum, void* handler_address);
int32_t sigreturn (void);
//
int32_t rand(void);
int32_t set_seed(uint32_t seed);



//---------------auxilliary functions----------------
extern pcb_t* get_pcb_ptr(uint32_t pid);
void _set_user_paging(uint32_t pid);
int32_t _parse_arg(const uint8_t* command, uint8_t* fname, uint8_t* args);

#endif /*_SYSTEMCALL_H */


