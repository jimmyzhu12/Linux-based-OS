/* lib.c - Some basic library functions (printf, strlen, etc.)
 * vim:ts=4 noexpandtab */

#include "lib.h"
#include "cp5.h"
#include "macros.h"

#define ATTRIB      0x7

// static int screen_x;
// static int screen_y;
static char* video_mem = (char *)VIDEO;

// static int terminal_lines;  // the lines currently in the current command



// refered from osdev
// cited from https://wiki.osdev.org/Text_Mode_Cursor
/* void enable_cursor(uint8_t cursor_start, uint8_t cursor_end);
 * Inputs: uint8_t cursor_start, uint8_t cursor_end : the start scan line and end scanline of the cursor
 * Return Value: none
 * Function: enale the cursor */
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
	return;
    outb(0x0A, 0x3D4);
    outb((inb(0x3D5) & 0xC0) | cursor_start, 0x3D5);
    outb(0x0B, 0x3D4);
    outb((inb(0x3D5) & 0xE0) | cursor_end, 0x3D5);
}

// referd from osdev 
// cited from https://wiki.osdev.org/Text_Mode_Cursor
/* void update_cursor(int x, int y);
 * Inputs: int x, int y :desired cursor location
 * Return Value: none
 * Function: updates the cursor */
void update_cursor(int x, int y)
{
	return;
    uint16_t pos = y * NUM_COLS + x;
    outb(0x0F, 0x3D4);
    outb((uint8_t) (pos & 0xFF), 0x3D5);
    outb(0x0E, 0x3D4);
    outb((uint8_t) ((pos >> 8) & 0xFF), 0x3D5);
}

/* void clear(void);
 * Inputs: void
 * Return Value: none
 * Function: Clears video memory */
void clear(void) {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
        *(uint8_t *)(video_mem + (i << 1) + 1) = ATTRIB;
    }
}

/* void clear_reset_cursor(void);
 * Inputs: void
 * Return Value: none
 * Function: Clears video memory and set the startpoint */
void clear_reset_cursor(void) {
    // turn the video map back to original
    set_kernel_map_for_copying();
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
        *(uint8_t *)(video_mem + (i << 1) + 1) = ATTRIB;
    }
    terminals[cur_terminal].cursor_x = 0;
    terminals[cur_terminal].cursor_y = 0;
    update_cursor(0, 0);
    // reset video map
    after_setting_run_reset_kernel_video_map();
}

/* void back_space_clear(void);
 * Inputs: void
 * Return Value: none
 * Function: clear the last char and reset the cursor */
void back_space_clear(void) {

    if (terminals[cur_terminal].cursor_x == 0) {
        if (terminals[cur_terminal].terminal_lines == 0)
            return;        // if nothing has been printed for this line, just return
        else
        {
            terminals[cur_terminal].cursor_x = NUM_COLS - 1;
            terminals[cur_terminal].cursor_y -= 1;
            terminals[cur_terminal].terminal_lines -= 1;
            putc_original(' ');
            terminals[cur_terminal].cursor_x = NUM_COLS - 1;
            terminals[cur_terminal].cursor_y -= 1;
            update_cursor(terminals[cur_terminal].cursor_x, terminals[cur_terminal].cursor_y);
            return;
        }
    }
    terminals[cur_terminal].cursor_x--; // move back word screen_x

    putc_original(' '); // print space 
    terminals[cur_terminal].cursor_x--;     // and then decrease screen_x for us to continue printing
    update_cursor(terminals[cur_terminal].cursor_x, terminals[cur_terminal].cursor_y);

}

/* Standard printf().
 * Only supports the following format strings:
 * %%  - print a literal '%' character
 * %x  - print a number in hexadecimal
 * %u  - print a number as an unsigned integer
 * %d  - print a number as a signed integer
 * %c  - print a character
 * %s  - print a string
 * %#x - print a number in 32-bit aligned hexadecimal, i.e.
 *       print 8 hexadecimal digits, zero-padded on the left.
 *       For example, the hex number "E" would be printed as
 *       "0000000E".
 *       Note: This is slightly different than the libc specification
 *       for the "#" modifier (this implementation doesn't add a "0x" at
 *       the beginning), but I think it's more flexible this way.
 *       Also note: %x is the only conversion specifier that can use
 *       the "#" modifier to alter output. */
int32_t printf(int8_t *format, ...) {

    /* Pointer to the format string */
    int8_t* buf = format;

    /* Stack pointer for the other parameters */
    int32_t* esp = (void *)&format;
    esp++;

    while (*buf != '\0') {
        switch (*buf) {
            case '%':
                {
                    int32_t alternate = 0;
                    buf++;

format_char_switch:
                    /* Conversion specifiers */
                    switch (*buf) {
                        /* Print a literal '%' character */
                        case '%':
                            putc('%');
                            break;

                        /* Use alternate formatting */
                        case '#':
                            alternate = 1;
                            buf++;
                            /* Yes, I know gotos are bad.  This is the
                             * most elegant and general way to do this,
                             * IMHO. */
                            goto format_char_switch;

                        /* Print a number in hexadecimal form */
                        case 'x':
                            {
                                int8_t conv_buf[64];
                                if (alternate == 0) {
                                    itoa(*((uint32_t *)esp), conv_buf, 16);
                                    puts(conv_buf);
                                } else {
                                    int32_t starting_index;
                                    int32_t i;
                                    itoa(*((uint32_t *)esp), &conv_buf[8], 16);
                                    i = starting_index = strlen(&conv_buf[8]);
                                    while(i < 8) {
                                        conv_buf[i] = '0';
                                        i++;
                                    }
                                    puts(&conv_buf[starting_index]);
                                }
                                esp++;
                            }
                            break;

                        /* Print a number in unsigned int form */
                        case 'u':
                            {
                                int8_t conv_buf[36];
                                itoa(*((uint32_t *)esp), conv_buf, 10);
                                puts(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a number in signed int form */
                        case 'd':
                            {
                                int8_t conv_buf[36];
                                int32_t value = *((int32_t *)esp);
                                if(value < 0) {
                                    conv_buf[0] = '-';
                                    itoa(-value, &conv_buf[1], 10);
                                } else {
                                    itoa(value, conv_buf, 10);
                                }
                                puts(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a single character */
                        case 'c':
                            putc((uint8_t) *((int32_t *)esp));
                            esp++;
                            break;

                        /* Print a NULL-terminated string */
                        case 's':
                            puts(*((int8_t **)esp));
                            esp++;
                            break;

                        default:
                            break;
                    }

                }
                break;

            default:
                putc(*buf);
                break;
        }
        buf++;
    }
    return (buf - format);
}

/* int32_t puts(int8_t* s);
 *   Inputs: int_8* s = pointer to a string of characters
 *   Return Value: Number of bytes written
 *    Function: Output a string to the console */
int32_t puts(int8_t* s) {
    register int32_t index = 0;
    while (s[index] != '\0') {
        putc(s[index]);
        index++;
    }
    return index;
}

/* void putc(uint8_t c);
 * Inputs: uint_8* c = character to print
 * Return Value: void
 *  Function: Output a character to the console */
void putc(uint8_t c) {
    if(c == '\n' || c == '\r') {
        if (terminals[run_terminal].cursor_y <= 23) {
            terminals[run_terminal].cursor_y++;
            terminals[run_terminal].cursor_x = 0;
            // cp5 modified
            update_cursor_remotely(terminals[run_terminal].cursor_x, terminals[run_terminal].cursor_y);
        } else {
            int i;   // iterate row
            int j;   // iterate col
            for (i = 0; i < 24; i++) {
                for (j = 0; j < 80; j++) {
                    *(uint8_t *)(video_mem + ((NUM_COLS * i + j) << 1)) = *(uint8_t *)(video_mem + ((NUM_COLS * (i + 1) + j) << 1));
                    *(uint8_t *)(video_mem + ((NUM_COLS * i + j) << 1) + 1) = ATTRIB;
                }
            }
            for (j = 0; j < 80; j++) {
                *(uint8_t *)(video_mem + ((NUM_COLS * 24 + j) << 1)) = ' ';
                *(uint8_t *)(video_mem + ((NUM_COLS * 24 + j) << 1) + 1) = ATTRIB;
            }
            terminals[run_terminal].cursor_y = 24;
            terminals[run_terminal].cursor_x = 0;
            // cp5 modified
            update_cursor_remotely(terminals[run_terminal].cursor_x, terminals[run_terminal].cursor_y);

        }

    } else {
        *(uint8_t *)(video_mem + ((NUM_COLS * terminals[run_terminal].cursor_y + terminals[run_terminal].cursor_x) << 1)) = c;
        *(uint8_t *)(video_mem + ((NUM_COLS * terminals[run_terminal].cursor_y + terminals[run_terminal].cursor_x) << 1) + 1) = ATTRIB;
        terminals[run_terminal].cursor_x++;
        
        if (terminals[run_terminal].cursor_x == 80) {
            terminals[run_terminal].cursor_y = terminals[run_terminal].cursor_y + 1;
            terminals[run_terminal].terminal_lines += 1;

            //screen_x = 0;
        }
        terminals[run_terminal].cursor_x %= NUM_COLS;      // reset 
        
        if (terminals[run_terminal].cursor_y >= 25) {
            int i;   // iterate row
            int j;   // iterate col
            for (i = 0; i < 24; i++) {
                for (j = 0; j < 80; j++) {
                    *(uint8_t *)(video_mem + ((NUM_COLS * i + j) << 1)) = *(uint8_t *)(video_mem + ((NUM_COLS * (i + 1) + j) << 1));
                    *(uint8_t *)(video_mem + ((NUM_COLS * i + j) << 1) + 1) = ATTRIB;
                }
            }
            for (j = 0; j < 80; j++) {
                *(uint8_t *)(video_mem + ((NUM_COLS * 24 + j) << 1)) = ' ';
                *(uint8_t *)(video_mem + ((NUM_COLS * 24 + j) << 1) + 1) = ATTRIB;
            }
            terminals[run_terminal].cursor_y = 24;
            
        }
        // cp5 modified
        update_cursor_remotely(terminals[run_terminal].cursor_x, terminals[run_terminal].cursor_y);
        terminals[run_terminal].cursor_x %= NUM_COLS;      // reset
        terminals[run_terminal].cursor_y = (terminals[run_terminal].cursor_y + (terminals[run_terminal].cursor_x / NUM_COLS)) % NUM_ROWS; // this line restarts y to 0 if it exceeds 24
    }
}


/* void putc_original(uint8_t c); always writes to the current displaying terminal
 * Inputs: uint_8* c = character to print
 * Return Value: void
 *  Function: Output a character to the console */
void putc_original(uint8_t c) {
    set_kernel_map_for_copying();
    char* video_mem_ori;
    video_mem_ori = (char *) VIDEO;
    if(c == '\n' || c == '\r') {
        if (terminals[cur_terminal].cursor_y <= 23) {
            terminals[cur_terminal].cursor_y++;
            terminals[cur_terminal].cursor_x = 0;
            update_cursor(terminals[cur_terminal].cursor_x, terminals[cur_terminal].cursor_y);
        } else {
            int i;   // iterate row
            int j;   // iterate col
            for (i = 0; i < 24; i++) {
                for (j = 0; j < 80; j++) {
                    *(uint8_t *)(video_mem_ori + ((NUM_COLS * i + j) << 1)) = *(uint8_t *)(video_mem_ori + ((NUM_COLS * (i + 1) + j) << 1));
                    *(uint8_t *)(video_mem_ori + ((NUM_COLS * i + j) << 1) + 1) = ATTRIB;
                }
            }
            for (j = 0; j < 80; j++) {
                *(uint8_t *)(video_mem_ori + ((NUM_COLS * 24 + j) << 1)) = ' ';
                *(uint8_t *)(video_mem_ori + ((NUM_COLS * 24 + j) << 1) + 1) = ATTRIB;
            }
            terminals[cur_terminal].cursor_y = 24;
            terminals[cur_terminal].cursor_x = 0;
            update_cursor(terminals[cur_terminal].cursor_x, terminals[cur_terminal].cursor_y);

        }

    } else {
        *(uint8_t *)(video_mem_ori + ((NUM_COLS * terminals[cur_terminal].cursor_y + terminals[cur_terminal].cursor_x) << 1)) = c;
        *(uint8_t *)(video_mem_ori + ((NUM_COLS * terminals[cur_terminal].cursor_y + terminals[cur_terminal].cursor_x) << 1) + 1) = ATTRIB;
        terminals[cur_terminal].cursor_x++;
        
        if (terminals[cur_terminal].cursor_x == 80) {
            terminals[cur_terminal].cursor_y = terminals[cur_terminal].cursor_y + 1;
            terminals[cur_terminal].terminal_lines += 1;

            //screen_x = 0;
        }
        terminals[cur_terminal].cursor_x %= NUM_COLS;      // reset 
        
        if (terminals[cur_terminal].cursor_y >= 25) {
            int i;   // iterate row
            int j;   // iterate col
            for (i = 0; i < 24; i++) {
                for (j = 0; j < 80; j++) {
                    *(uint8_t *)(video_mem_ori + ((NUM_COLS * i + j) << 1)) = *(uint8_t *)(video_mem_ori + ((NUM_COLS * (i + 1) + j) << 1));
                    *(uint8_t *)(video_mem_ori + ((NUM_COLS * i + j) << 1) + 1) = ATTRIB;
                }
            }
            for (j = 0; j < 80; j++) {
                *(uint8_t *)(video_mem_ori + ((NUM_COLS * 24 + j) << 1)) = ' ';
                *(uint8_t *)(video_mem_ori + ((NUM_COLS * 24 + j) << 1) + 1) = ATTRIB;
            }
            terminals[cur_terminal].cursor_y = 24;
            
        }
        update_cursor(terminals[cur_terminal].cursor_x, terminals[cur_terminal].cursor_y);
        terminals[cur_terminal].cursor_x %= NUM_COLS;      // reset
        terminals[cur_terminal].cursor_y = (terminals[cur_terminal].cursor_y + (terminals[cur_terminal].cursor_x / NUM_COLS)) % NUM_ROWS; // this line restarts y to 0 if it exceeds 24
    }
    after_setting_run_reset_kernel_video_map();
}

/* int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix);
 * Inputs: uint32_t value = number to convert
 *            int8_t* buf = allocated buffer to place string in
 *          int32_t radix = base system. hex, oct, dec, etc.
 * Return Value: number of bytes written
 * Function: Convert a number to its ASCII representation, with base "radix" */
int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix) {
    static int8_t lookup[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int8_t *newbuf = buf;
    int32_t i;
    uint32_t newval = value;

    /* Special case for zero */
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }

    /* Go through the number one place value at a time, and add the
     * correct digit to "newbuf".  We actually add characters to the
     * ASCII string from lowest place value to highest, which is the
     * opposite of how the number should be printed.  We'll reverse the
     * characters later. */
    while (newval > 0) {
        i = newval % radix;
        *newbuf = lookup[i];
        newbuf++;
        newval /= radix;
    }

    /* Add a terminating NULL */
    *newbuf = '\0';

    /* Reverse the string and return */
    return strrev(buf);
}

/* int8_t* strrev(int8_t* s);
 * Inputs: int8_t* s = string to reverse
 * Return Value: reversed string
 * Function: reverses a string s */
int8_t* strrev(int8_t* s) {
    register int8_t tmp;
    register int32_t beg = 0;
    register int32_t end = strlen(s) - 1;

    while (beg < end) {
        tmp = s[end];
        s[end] = s[beg];
        s[beg] = tmp;
        beg++;
        end--;
    }
    return s;
}

/* uint32_t strlen(const int8_t* s);
 * Inputs: const int8_t* s = string to take length of
 * Return Value: length of string s
 * Function: return length of string s */
uint32_t strlen(const int8_t* s) {
    register uint32_t len = 0;
    while (s[len] != '\0')
        len++;
    return len;
}

/* void* memset(void* s, int32_t c, uint32_t n);
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive bytes of pointer s to value c */
void* memset(void* s, int32_t c, uint32_t n) {
    c &= 0xFF;
    asm volatile ("                 \n\
            .memset_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memset_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memset_aligned \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memset_top     \n\
            .memset_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     stosl           \n\
            .memset_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memset_done    \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%edx       \n\
            jmp     .memset_bottom  \n\
            .memset_done:           \n\
            "
            :
            : "a"(c << 24 | c << 16 | c << 8 | c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memset_word(void* s, int32_t c, uint32_t n);
 * Description: Optimized memset_word
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set lower 16 bits of n consecutive memory locations of pointer s to value c */
void* memset_word(void* s, int32_t c, uint32_t n) {
    asm volatile ("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosw           \n\
            "
            :
            : "a"(c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memset_dword(void* s, int32_t c, uint32_t n);
 * Inputs:    void* s = pointer to memory
 *          int32_t c = value to set memory to
 *         uint32_t n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive memory locations of pointer s to value c */
void* memset_dword(void* s, int32_t c, uint32_t n) {
    asm volatile ("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosl           \n\
            "
            :
            : "a"(c), "D"(s), "c"(n)
            : "edx", "memory", "cc"
    );
    return s;
}

/* void* memcpy(void* dest, const void* src, uint32_t n);
 * Inputs:      void* dest = destination of copy
 *         const void* src = source of copy
 *              uint32_t n = number of byets to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of src to dest */
void* memcpy(void* dest, const void* src, uint32_t n) {
    asm volatile ("                 \n\
            .memcpy_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memcpy_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memcpy_aligned \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memcpy_top     \n\
            .memcpy_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     movsl           \n\
            .memcpy_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memcpy_done    \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%edx       \n\
            jmp     .memcpy_bottom  \n\
            .memcpy_done:           \n\
            "
            :
            : "S"(src), "D"(dest), "c"(n)
            : "eax", "edx", "memory", "cc"
    );
    return dest;
}

/* void* memmove(void* dest, const void* src, uint32_t n);
 * Description: Optimized memmove (used for overlapping memory areas)
 * Inputs:      void* dest = destination of move
 *         const void* src = source of move
 *              uint32_t n = number of byets to move
 * Return Value: pointer to dest
 * Function: move n bytes of src to dest */
void* memmove(void* dest, const void* src, uint32_t n) {
    asm volatile ("                             \n\
            movw    %%ds, %%dx                  \n\
            movw    %%dx, %%es                  \n\
            cld                                 \n\
            cmp     %%edi, %%esi                \n\
            jae     .memmove_go                 \n\
            leal    -1(%%esi, %%ecx), %%esi     \n\
            leal    -1(%%edi, %%ecx), %%edi     \n\
            std                                 \n\
            .memmove_go:                        \n\
            rep     movsb                       \n\
            "
            :
            : "D"(dest), "S"(src), "c"(n)
            : "edx", "memory", "cc"
    );
    return dest;
}

/* int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n)
 * Inputs: const int8_t* s1 = first string to compare
 *         const int8_t* s2 = second string to compare
 *               uint32_t n = number of bytes to compare
 * Return Value: A zero value indicates that the characters compared
 *               in both strings form the same string.
 *               A value greater than zero indicates that the first
 *               character that does not match has a greater value
 *               in str1 than in str2; And a value less than zero
 *               indicates the opposite.
 * Function: compares string 1 and string 2 for equality */
int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n) {
    int32_t i;
    for (i = 0; i < n; i++) {
        if ((s1[i] != s2[i]) || (s1[i] == '\0') /* || s2[i] == '\0' */) {

            /* The s2[i] == '\0' is unnecessary because of the short-circuit
             * semantics of 'if' expressions in C.  If the first expression
             * (s1[i] != s2[i]) evaluates to false, that is, if s1[i] ==
             * s2[i], then we only need to test either s1[i] or s2[i] for
             * '\0', since we know they are equal. */
            return s1[i] - s2[i];
        }
    }
    return 0;
}

/* int8_t* strcpy(int8_t* dest, const int8_t* src)
 * Inputs:      int8_t* dest = destination string of copy
 *         const int8_t* src = source string of copy
 * Return Value: pointer to dest
 * Function: copy the source string into the destination string */
int8_t* strcpy(int8_t* dest, const int8_t* src) {
    int32_t i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

/* int8_t* strcpy(int8_t* dest, const int8_t* src, uint32_t n)
 * Inputs:      int8_t* dest = destination string of copy
 *         const int8_t* src = source string of copy
 *                uint32_t n = number of bytes to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of the source string into the destination string */
int8_t* strncpy(int8_t* dest, const int8_t* src, uint32_t n) {
    int32_t i = 0;
    while (src[i] != '\0' && i < n) {
        dest[i] = src[i];
        i++;
    }
    while (i < n) {
        dest[i] = '\0';
        i++;
    }
    return dest;
}

/* void reset_terminal_lines(void)
 * Inputs: void
 * Return Value: void
 * Function: reset the variable terminal_lines back to 0 */
void reset_terminal_lines(void)
{
    // if bug appears in cp5, check here!
    terminals[cur_terminal].terminal_lines = 0;
}

/* void test_interrupts(void)
 * Inputs: void
 * Return Value: void
 * Function: increments video memory. To be used to test rtc */
void test_interrupts(void) {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        video_mem[i << 1]++;
    }
}

/* void clear_terminal_memory(void);
 * Inputs: physical_memory_address - the physical start address of that terminal's video memory
 * Return Value: none
 * Function: Clears video memory of a terminal */
void clear_terminal_memory(uint8_t* physical_memory_address) {
    int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(physical_memory_address + (i << 1)) = ' ';
        *(uint8_t *)(physical_memory_address + (i << 1) + 1) = ATTRIB;
    }
}

// referd from osdev. Modified from update_cursor(). This function is able to update a cursor that is not on the current displaying screen.
// cited from https://wiki.osdev.org/Text_Mode_Cursor
/* void update_cursor_remotely(int x, int y);
 * Inputs: int x, int y :desired cursor location
 * Return Value: none
 * Function: updates the cursor in the desired terminal */
void update_cursor_remotely(int x, int y)
{
    if (cur_terminal == run_terminal)
    {
        update_cursor(x, y);
    }
    else
    {
        terminals[run_terminal].cursor_x = x;
        terminals[run_terminal].cursor_y = y;
    }
}

/* revised printf(): always print on the current terminal
 * Only supports the following format strings:
 * %%  - print a literal '%' character
 * %x  - print a number in hexadecimal
 * %u  - print a number as an unsigned integer
 * %d  - print a number as a signed integer
 * %c  - print a character
 * %s  - print a string
 * %#x - print a number in 32-bit aligned hexadecimal, i.e.
 *       print 8 hexadecimal digits, zero-padded on the left.
 *       For example, the hex number "E" would be printed as
 *       "0000000E".
 *       Note: This is slightly different than the libc specification
 *       for the "#" modifier (this implementation doesn't add a "0x" at
 *       the beginning), but I think it's more flexible this way.
 *       Also note: %x is the only conversion specifier that can use
 *       the "#" modifier to alter output. */
int32_t printf_original(int8_t *format, ...) {

    /* Pointer to the format string */
    int8_t* buf = format;

    /* Stack pointer for the other parameters */
    int32_t* esp = (void *)&format;
    esp++;

    while (*buf != '\0') {
        switch (*buf) {
            case '%':
                {
                    int32_t alternate = 0;
                    buf++;

format_char_switch:
                    /* Conversion specifiers */
                    switch (*buf) {
                        /* Print a literal '%' character */
                        case '%':
                            putc_original('%');
                            break;

                        /* Use alternate formatting */
                        case '#':
                            alternate = 1;
                            buf++;
                            /* Yes, I know gotos are bad.  This is the
                             * most elegant and general way to do this,
                             * IMHO. */
                            goto format_char_switch;

                        /* Print a number in hexadecimal form */
                        case 'x':
                            {
                                int8_t conv_buf[64];
                                if (alternate == 0) {
                                    itoa(*((uint32_t *)esp), conv_buf, 16);
                                    puts_original(conv_buf);
                                } else {
                                    int32_t starting_index;
                                    int32_t i;
                                    itoa(*((uint32_t *)esp), &conv_buf[8], 16);
                                    i = starting_index = strlen(&conv_buf[8]);
                                    while(i < 8) {
                                        conv_buf[i] = '0';
                                        i++;
                                    }
                                    puts_original(&conv_buf[starting_index]);
                                }
                                esp++;
                            }
                            break;

                        /* Print a number in unsigned int form */
                        case 'u':
                            {
                                int8_t conv_buf[36];
                                itoa(*((uint32_t *)esp), conv_buf, 10);
                                puts_original(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a number in signed int form */
                        case 'd':
                            {
                                int8_t conv_buf[36];
                                int32_t value = *((int32_t *)esp);
                                if(value < 0) {
                                    conv_buf[0] = '-';
                                    itoa(-value, &conv_buf[1], 10);
                                } else {
                                    itoa(value, conv_buf, 10);
                                }
                                puts_original(conv_buf);
                                esp++;
                            }
                            break;

                        /* Print a single character */
                        case 'c':
                            putc_original((uint8_t) *((int32_t *)esp));
                            esp++;
                            break;

                        /* Print a NULL-terminated string */
                        case 's':
                            puts_original(*((int8_t **)esp));
                            esp++;
                            break;

                        default:
                            break;
                    }

                }
                break;

            default:
                putc_original(*buf);
                break;
        }
        buf++;
    }
    return (buf - format);
}

/* int32_t puts_original(int8_t* s): always puts on the current terminal;
 *   Inputs: int_8* s = pointer to a string of characters
 *   Return Value: Number of bytes written
 *    Function: Output a string to the console */
int32_t puts_original(int8_t* s) {
    register int32_t index = 0;
    while (s[index] != '\0') {
        putc_original(s[index]);
        index++;
    }
    return index;
}


// referenced from https://wiki.osdev.org/Network_Stack
uint16_t switch_endian16(uint16_t nb)
{
    return (nb >> 8) | (nb << 8);
}

// referenced from https://wiki.osdev.org/Network_Stack
uint32_t switch_endian32(uint32_t nb)
{
    return    ((nb>>24)&0xFF)      |
              ((nb<<8)&0xFF0000)   |
              ((nb>>8)&0xFF00)     |
              ((nb<<24)&0xFF000000);

}


uint32_t htonl(uint32_t hostlong)
{
    return switch_endian32(hostlong);
}

uint16_t htons(uint16_t hostshort)
{
   return switch_endian16(hostshort); 
}

uint32_t ntohl(uint32_t netlong)
{
    return switch_endian32(netlong);
}

uint16_t ntohs(uint16_t netshort)
{
    return switch_endian16(netshort);
}

uint8_t adjust_endian_in_one_byte(uint8_t byte, uint8_t segment_pos)
// example:    
//  __7___6___5___4___3___2___1___0__               __7___6___5___4___3___2___1___0__
//  |   |   |   |   |   /   |   |   |       ===>    /   |   |   |   |   |   |   |   /
//                 segment_pos: 3
{
    uint8_t adjusted_byte = (0x00 | (byte << (8 - segment_pos)) | (byte >> segment_pos));
    return adjusted_byte;
}





uint32_t mac_addr_cmp(uint8_t* mac1, uint8_t* mac2)
{
    return strncmp_unsigned(mac1, mac2, 6);
}

uint32_t ip_addr_cmp(uint8_t* ip1, uint8_t* ip2)
{
    return strncmp_unsigned(ip1, ip2, 4);
}


uint32_t strncmp_unsigned(const uint8_t* s1, const uint8_t* s2, uint32_t n) {
    uint32_t i;
    for (i = 0; i < n; i++) {
        if ((s1[i] != s2[i]) || (s1[i] == '\0') /* || s2[i] == '\0' */) {

            /* The s2[i] == '\0' is unnecessary because of the short-circuit
             * semantics of 'if' expressions in C.  If the first expression
             * (s1[i] != s2[i]) evaluates to false, that is, if s1[i] ==
             * s2[i], then we only need to test either s1[i] or s2[i] for
             * '\0', since we know they are equal. */
            return s1[i] - s2[i];
        }
    }
    return 0;
}



void network_memcpy_uint8ptr(uint8_t* dest, uint8_t* src, uint32_t length)
{
    int i = 0;
    for (i = 0; i < length; i++)
    {
        dest[i] = src[i];
    }
}


