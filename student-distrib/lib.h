/* lib.h - Defines for useful library functions
 * vim:ts=4 noexpandtab
 */

#ifndef _LIB_H
#define _LIB_H

#include "types.h"
#include "macros.h"

int32_t printf(int8_t *format, ...);
void putc(uint8_t c);
int32_t puts(int8_t *s);
int8_t *itoa(uint32_t value, int8_t* buf, int32_t radix);
int8_t *strrev(int8_t* s);
uint32_t strlen(const int8_t* s);
void clear(void);
//Add
void back_space_clear(void);
// Added
void clear_reset_cursor(void);
void reset_terminal_lines(void);

void* memset(void* s, int32_t c, uint32_t n);
void* memset_word(void* s, int32_t c, uint32_t n);
void* memset_dword(void* s, int32_t c, uint32_t n);
void* memcpy(void* dest, const void* src, uint32_t n);
void* memmove(void* dest, const void* src, uint32_t n);
int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n);
int8_t* strcpy(int8_t* dest, const int8_t*src);
int8_t* strncpy(int8_t* dest, const int8_t*src, uint32_t n);
//add
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end); // defined enable cursor
//add
void update_cursor(int x, int y);
/* Userspace address-check functions */
int32_t bad_userspace_addr(const void* addr, int32_t len);
int32_t safe_strncpy(int8_t* dest, const int8_t* src, int32_t n);

// Added
void test_interrupts(void);

// Added for cp5
void clear_terminal_memory(uint8_t* physical_memory_address);
void update_cursor_remotely(int x, int y);
void putc_original(uint8_t c);
int32_t printf_original(int8_t *format, ...);
int32_t puts_original(int8_t* s);

// CP5: multi-terminals
// struct for terminal
typedef struct terminal_t{
    uint32_t id;   // terminal id. 0, 1 or 2.
    uint32_t booted;        // 1 if the terminal has already started before
    uint8_t* video_mem_phys;    // physical memory for the video content in this terminal
    uint32_t cursor_x;      // x coordinate of the cursor
    uint32_t cursor_y;      // y coordinate of the cursor
    uint32_t num_process;           // the number of processes running in this terminal
    uint32_t pids[MAX_PROCESS];     // the process ids in this terminal. -1 for not running
    uint32_t terminal_lines;
} terminal_t;

// the terminals
terminal_t terminals[NUMBER_OF_TERMINAL + 2];
int welcome_printed[NUMBER_OF_TERMINAL];

uint32_t cur_terminal;   // 0, 1 or 2, this is the displaying terminal
uint32_t run_terminal;   // 0, 1 or 2, this is the running terminal.
uint32_t halt_terminal;
uint32_t recursive_halt_terminal;
uint32_t recursive_halt_counter;


// extra credit: network, referenced from https://wiki.osdev.org/Network_Stack
uint16_t switch_endian16(uint16_t nb);
uint32_t switch_endian32(uint32_t nb);

// Reference from Beej's Guide to Network Programming section 9.12
uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);
uint8_t adjust_endian_in_one_byte(uint8_t byte, uint8_t segment_pos);

// address compare
uint32_t strncmp_unsigned(const uint8_t* s1, const uint8_t* s2, uint32_t n);
uint32_t mac_addr_cmp(uint8_t* mac1, uint8_t* mac2);
uint32_t ip_addr_cmp(uint8_t* ip1, uint8_t* ip2);
void network_memcpy_uint8ptr(uint8_t* dest, uint8_t* src, uint32_t length);



/* Port read functions */
/* Inb reads a byte and returns its value as a zero-extended 32-bit
 * unsigned int */
static inline uint32_t inb(port) {
    uint32_t val;
    asm volatile ("             \n\
            xorl %0, %0         \n\
            inb  (%w1), %b0     \n\
            "
            : "=a"(val)
            : "d"(port)
            : "memory"
    );
    return val;
}

/* Reads two bytes from two consecutive ports, starting at "port",
 * concatenates them little-endian style, and returns them zero-extended
 * */
static inline uint32_t inw(port) {
    uint32_t val;
    asm volatile ("             \n\
            xorl %0, %0         \n\
            inw  (%w1), %w0     \n\
            "
            : "=a"(val)
            : "d"(port)
            : "memory"
    );
    return val;
}

/* Reads four bytes from four consecutive ports, starting at "port",
 * concatenates them little-endian style, and returns them */
static inline uint32_t inl(port) {
    uint32_t val;
    asm volatile ("inl (%w1), %0"
            : "=a"(val)
            : "d"(port)
            : "memory"
    );
    return val;
}

/* Writes a byte to a port */
#define outb(data, port)                \
do {                                    \
    asm volatile ("outb %b1, (%w0)"     \
            :                           \
            : "d"(port), "a"(data)      \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Writes two bytes to two consecutive ports */
#define outw(data, port)                \
do {                                    \
    asm volatile ("outw %w1, (%w0)"     \
            :                           \
            : "d"(port), "a"(data)      \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Writes four bytes to four consecutive ports */
#define outl(data, port)                \
do {                                    \
    asm volatile ("outl %1, (%w0)"     \
            :                           \
            : "d"(port), "a"(data)      \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Clear interrupt flag - disables interrupts on this processor */
#define cli()                           \
do {                                    \
    asm volatile ("cli"                 \
            :                           \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Save flags and then clear interrupt flag
 * Saves the EFLAGS register into the variable "flags", and then
 * disables interrupts on this processor */
#define cli_and_save(flags)             \
do {                                    \
    asm volatile ("                   \n\
            pushfl                    \n\
            popl %0                   \n\
            cli                       \n\
            "                           \
            : "=r"(flags)               \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Set interrupt flag - enable interrupts on this processor */
#define sti()                           \
do {                                    \
    asm volatile ("sti"                 \
            :                           \
            :                           \
            : "memory", "cc"            \
    );                                  \
} while (0)

/* Restore flags
 * Puts the value in "flags" into the EFLAGS register.  Most often used
 * after a cli_and_save_flags(flags) */
#define restore_flags(flags)            \
do {                                    \
    asm volatile ("                   \n\
            pushl %0                  \n\
            popfl                     \n\
            "                           \
            :                           \
            : "r"(flags)                \
            : "memory", "cc"            \
    );                                  \
} while (0)

#endif /* _LIB_H */
