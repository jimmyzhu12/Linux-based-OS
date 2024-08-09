#include "terminal.h"
#include "Network/udp.h"

/*
 * terminal_read()
 *   DESCRIPTION: This function reads the read_buffer that is filled when pressed enter to a given buffer
 *   INPUTS: uint32_t fd: Not used here
 *           void * buf: the buffer pointer to be copied into
 *           uint32_t nbytes: Not used here
 *   OUTPUTS: none
 *   RETURN VALUE: the number of bytes copied
 *   SIDE EFFECTS: copy to the given buffer
 */
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes) {
    int i;                  // buf iterator

    reset_terminal_lines();
    while ((!enter_flag) || (cur_terminal != run_terminal)) {}  // halts until I get an enter pressed
    for (i = 0; i <= 127; i++) {        // try to find the \n
        if (read_buffer[cur_terminal][i] == '\n') {
           break;
        } 
    }
    enter_flag = 0;
    memcpy(buf, read_buffer[cur_terminal], i + 1);    // copy to given buffer

    return i + 1;
}

/*
 * terminal_write()
 *   DESCRIPTION: This function displays the context in the given buffer to the screen
 *   INPUTS: uint32_t fd: Not used here
 *           void * buf: the buffer pointer to be displayed
 *           uint32_t nbytes: the number of bytes to display
 *   OUTPUTS: prints to the screen of buf value
 *   RETURN VALUE: the number of bytes displayed 
 *   SIDE EFFECTS: prints to the screen
 */
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes) {
    int i;
    // int put_on_current_screen = 0;
    if (buf == NULL) {                  // if the buf is null, return -1
        return -1;
    }
    char * tmp;
    tmp = (char *)buf;

    // sti();
    
    for (i = 0; i < nbytes; i++) {      // putc the context in the given buffer
        putc(tmp[i]);
    }

    return nbytes;
}

/*
 * terminal_open()
 *   DESCRIPTION: Not used
 *   INPUTS: const uint8_t* filename: Not used here
 *   OUTPUTS: none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: none
 */
int32_t terminal_open (const uint8_t* filename) {
    return 0;  // always success
}

/*
 * terminal_close()
 *   DESCRIPTION: Not used
 *   INPUTS: uint32_t fd: Not used here
 *   OUTPUTS: none
 *   RETURN VALUE: -1
 *   SIDE EFFECTS: none
 */
int32_t terminal_close (int32_t fd) {
    return -1;  // should not close
}
