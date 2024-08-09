#include "keyboard_handler.h"
#include "../SystemCall.h"
#include "../i8259.h"
#include "../lib.h"
#include "../cp5.h"
#include "../Scheduler.h"
#include "../gui/vga.h"
#include "../gui/gui.h"
#include "../Network/udp.h"
#include "../sound/piano.h"

#define Keyboard_IRQ_NUM    1
#define Keyboard_size 0x3A + 1
#define max_buffer_index  127
#define max_buffer_length  128
#define left_shift_press_scancode 0x2A
#define right_shift_press_scancode 0x36
#define left_shift_release_scancode 0xAA
#define right_shift_release_scancode 0x36 + 0x80
#define left_ctrl_press_scancode 0x1D
#define left_ctrl_release_scancode 0x9D
#define backspace_press_scancode 0x0E
#define enter_press_scancode 0x1C
#define caps_lock_press 0x3A
#define l_press_scancode 0x26
#define c_press_scancode 0x2E
#define char_hold_capacity 126

// CP5: multi-terminals
#define left_alt_press_scancode 0x38
#define left_alt_release_scancode 0xB8
#define f1_press_scancode 0x3B
#define f2_press_scancode 0x3C
#define f3_press_scancode 0x3D
#define f4_press_scancode 0x3E
#define f5_press_scancode 0x3F
#define f6_press_scancode 0x40

// extra credit: networking
#define n_press_scancode 0x31
#define i_press_scancode 0x17

// extra credit: piano
#define m_press_scancode 0x32
#define p_press_scancode 0x19

// extra credit: TAB
#define TAB_scancode 0x0f

// extra credit history
#define press_top 0x48
#define press_bot 0x50

// extra credit: recursive halt
#define q_press_scancode    0x10

int whether_record[3];

int depth;						// the depth in the history structure


/*
 * keyboard_init()
 *   DESCRIPTION: This function initliaze the keyboard irq and then clears all the buffers
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: keyboard irq turned on
 */
void keyboard_init(){
    enable_irq(Keyboard_IRQ_NUM);
    int i = 0;
    for(i = 0; i < NUMBER_OF_TERMINAL; i++)
    {
        buffer_c[i] = 0;
    }
    init_buffer();
    piano_on = 0;
    strcpy(file_names[0], "sigtest\0");
    strcpy(file_names[1], "shell\0");
    strcpy(file_names[2], "grep\0");
    strcpy(file_names[3], "syserr\0");
    strcpy(file_names[4], "rtc\0");
    strcpy(file_names[5], "fish\0");
    strcpy(file_names[6], "counter\0");
    strcpy(file_names[7], "pingpong\0");
    strcpy(file_names[8], "cat\0");
    strcpy(file_names[9], "ls\0");
    strcpy(file_names[10], "testprint\0");
    strcpy(file_names[11], "hello\0");
    // keyboard init need to initialize buffer both of them, which is just make them \0
}

// https://wiki.osdev.org/PS/2_Keyboard
// This table is used for lowercase characters
static unsigned char scancode_table[Keyboard_size] = { // the first one is an error code 
// if 0x00 it means i dont need to do anything for this in cp3.1
// scan code range from 0x00 to 0x32 for now to include all the numbers and characters 
    0x00, 0x00, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-',
    '=', 0x00, 0x00, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',   // these two 0 are backspace amnd tab 
    '[', ']', 0x00, 0x00, 'a', 's', 'd', 'f', 'g','h', 'j', 'k', 'l',    // these two 0 are enter and left control
    ';', '\'', '`', 0x00, '\\','z','x','c','v','b','n','m', ',', '.', '/',               // the 0 are ' left shift "\"
    0x00, 0x00, 0x00, ' ', 0x00
};
// This table is for uppercase character lookup table
static unsigned char upcase_scancode_table[Keyboard_size] = { // the first one is an error code 
// if 0x00 it means i dont need to do anything for this in cp3.1
// scan code range from 0x00 to 0x32 for now to include all the numbers and characters 
    0x00, 0x00, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_',
    '+', 0x00, 0x00, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',   // these two 0 are backspace amnd tab 
    '{', '}', 0x00, 0x00, 'A', 'S', 'D', 'F', 'G','H', 'J', 'K', 'L',    // these two 0 are enter and left control
    ':', '"', '~', 0x00, '|','Z','X','C','V','B','N','M', '<', '>', '?',               // the 0 are ' left shift "\"
    0x00, 0x00, 0x00, ' ', 0x00

};



// https://wiki.osdev.org/Text_Mode_Cursor this might help
static int shift_pressed; // 1 means pressed, 0 means released
static int caps_status; // 1 means upcase, 0 means lower case 
int ctrl_status; // 1 means pressed, 0 means released 0x1D

// CP5: multi-terminal
int alt_status;     // 1 means pressed, 0 means released


/*
 * copy_buffer()
 *   DESCRIPTION: This function copies the enter buffer into my read_buffer for future ues
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void copy_buffer() {                                        
    int i;
    for (i = 0; i <= max_buffer_index; i++) {
        
        read_buffer[cur_terminal][i] = enter_buffer[cur_terminal][i];         // copy enter buffer to read buffer
    }
}
/*
 * init_enter()
 *   DESCRIPTION: This function init enter buffer
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void init_enter() {
    int i;
    for (i = 0; i <= max_buffer_index; i++) {
        enter_buffer[run_terminal][i] = '\0';         // copy enter buffer to read buffer
    }
    buffer_c[run_terminal] = 0;
    terminals[run_terminal].terminal_lines = 0;
}
/*
 * init_buffer()
 *   DESCRIPTION: This function clears all buffer
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void init_buffer() {
    int i, j;
    for (i = 0; i <= max_buffer_index; i++) {
        for(j = 0; j <= NUMBER_OF_TERMINAL; j++)
        {
            read_buffer[j][i] = '\0';                     // clear read buffer
            enter_buffer[j][i] = '\0';                    // clear enter buffer
        }
    }
    
}
/*
 * handle_kb_interrupt()
 *   DESCRIPTION: This function handles keyboard interrupt, writes buffer, print to screen
 *   INPUTS: none
 *   OUTPUTS: displays to the screen
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints / writes to bufer
 */
void handle_kb_interrupt() {
    cli();
    uint32_t fetched_code = inb(KB_read_port); // read the scancode from the keyboard
    // debugging in cp5
    // printf("fetched_code: %x", fetched_code);
    

    // generally speaking, it is not a printable key, I do not need to put it onto the buffer
    if (fetched_code == left_shift_press_scancode || fetched_code == right_shift_press_scancode) { // if shift pressed
        shift_pressed = 1;                            // if shift is pressed, the shift select = 1
        send_eoi(Keyboard_IRQ_NUM);                   // end of interrupt
        return;   
    }
    // if shfit is  released, reset the shift_pressed indicator
    if (fetched_code == left_shift_release_scancode || fetched_code == (right_shift_release_scancode)) { 
        shift_pressed = 0;                            // if shift is release, the shift select = 0
        send_eoi(Keyboard_IRQ_NUM);
        return;   
    }
    // press ctrl sets the ctrl flag
    if (fetched_code == left_ctrl_press_scancode) { 
        ctrl_status = 1;                              // set ctrl flag
        send_eoi(Keyboard_IRQ_NUM);
        return;
    }
    // release ctrl resets the ctrl flag to 0
    if (fetched_code == left_ctrl_release_scancode) {  
        ctrl_status = 0;                              // reset ctrl flag
        send_eoi(Keyboard_IRQ_NUM);
        return;   
    }
    // CP5: multi-terminals
    // press alt sets the alt flag
    if (fetched_code == left_alt_press_scancode) { 
        alt_status = 1;                              // set ctrl flag
        send_eoi(Keyboard_IRQ_NUM);
        return;
    }
    // release alt resets the alt flag to 0
    if (fetched_code == left_alt_release_scancode) {  
        alt_status = 0;                              // reset ctrl flag
        send_eoi(Keyboard_IRQ_NUM);
        return;   
    }
    // press backspace (release does not matter) removes one char from screen and buffer
    if (fetched_code == backspace_press_scancode) { 
        if (buffer_c[cur_terminal] > 0) {
            back_space_clear();    // clear the current one char from screen
            enter_buffer[cur_terminal][buffer_c[cur_terminal]] = '\0';  // reset the last char entered to \0
            buffer_c[cur_terminal] -= 1;                  // pops back the buffer_count
        }

        send_eoi(Keyboard_IRQ_NUM);
        return;
    }
    // press enter goes to the next line on screen, and copies buffers for terminal read
    if (fetched_code == enter_press_scancode) { 
        enter_flag = 1;                             // for test termial read/ write purpose
        if (buffer_c[cur_terminal] <= max_buffer_index) {         // if it is not the last index, i need manually add \n
            enter_buffer[cur_terminal][buffer_c[cur_terminal]] = '\n';
        }
        copy_buffer();                              // make enter buffer go to read buffer
        if (whether_record[cur_terminal]) {
			record_buf();
		}
        putc_original('\n');                                 // go to next line
        int i;
        for (i = 0; i < max_buffer_length; i++) {
            enter_buffer[cur_terminal][i] = '\0';
        }
        buffer_c[cur_terminal] = 0;
        depth = 0;
        send_eoi(Keyboard_IRQ_NUM);
        return;
    }

    if (fetched_code == TAB_scancode) {
        
        int i;
        int headcount = 0;
        char * the_name;
        for (i = 0; i < 15; i++) {
            if(if_head_match(enter_buffer[cur_terminal], file_names[i])) {
                headcount++;
                the_name = file_names[i];
            }
        }
        if (headcount == 1) {
            while (buffer_c[cur_terminal] > 0) {
                back_space_clear();
                enter_buffer[cur_terminal][buffer_c[cur_terminal]] = '\0';
                buffer_c[cur_terminal]--;
            }
            
            for (i = 0; i < strlen(the_name); i++) {
                enter_buffer[cur_terminal][buffer_c[cur_terminal]] = the_name[i];
                buffer_c[cur_terminal]++;
                putc_original(the_name[i]);
            }
        }
        send_eoi(Keyboard_IRQ_NUM);
        return;

    }
    // ec: history
	if (fetched_code == press_top) {
		if (depth < REM) {
			if (prev_one_read[depth][cur_terminal][0] != '\0') {
				while (buffer_c[cur_terminal] > 0) {
					back_space_clear();
					enter_buffer[cur_terminal][buffer_c[cur_terminal]] = '\0';
					buffer_c[cur_terminal]--;
				}
				copy_into_enter();
				buffer_c[cur_terminal] = buffer_prev[depth][cur_terminal];
				int i;
				for (i = 0; i < buffer_c[cur_terminal]; i++) {
					putc_original(enter_buffer[cur_terminal][i]);
				}
				depth++;
				if (depth >= REM) {
					depth = REM - 1;		// currently depth 2 means out of size, so I need to discrad
				}
			}
		}
	}
	if (fetched_code == press_bot) {
		depth--;
		if (depth > -1) {
			while (buffer_c[cur_terminal] > 0) {
				back_space_clear();
				enter_buffer[cur_terminal][buffer_c[cur_terminal]] = '\0';
				buffer_c[cur_terminal]--;
			}
			copy_into_enter();
			buffer_c[cur_terminal] = buffer_prev[depth][cur_terminal];
			int i;
			for (i = 0; i < buffer_c[cur_terminal]; i++) {
				putc_original(enter_buffer[cur_terminal][i]);
			}
		}
		if (depth < 0) {
			depth = 0;
			while (buffer_c[cur_terminal] > 0) {
				back_space_clear();
				enter_buffer[cur_terminal][buffer_c[cur_terminal]] = '\0';
				buffer_c[cur_terminal]--;
			}
		}
	} 




    // if caps lock is pressed, I change the status of lock (uo to down, down to up)
    if (fetched_code == caps_lock_press) {  
        if (caps_status == 0) {
            caps_status = 1;
        } else if (caps_status == 1) {
            caps_status = 0;
        }
        send_eoi(Keyboard_IRQ_NUM);
        return;   
    }

    // left alt + F1
    if (alt_status == 1) {
        switch (fetched_code)
        {
        case f1_press_scancode:
            send_eoi(Keyboard_IRQ_NUM);
            disable_all();
            vga_setmode(G640x480x32K);
            enable_all();
            return;
        case f2_press_scancode:
            send_eoi(Keyboard_IRQ_NUM);
            disable_all();
            vga_setmode(G800x600x32K);
            enable_all();
            return;
        case f3_press_scancode:
            send_eoi(Keyboard_IRQ_NUM);
            disable_all();
            vga_setmode(G1024x768x32K);
            enable_all();
            return;
        case f4_press_scancode:
        case f5_press_scancode:
        case f6_press_scancode:
            send_eoi(Keyboard_IRQ_NUM);
            font_obj_init(fetched_code - f4_press_scancode);
            return;
        default:
            break;
        }
    }

    // this means it  exceeds our current table size, and we must do nothing in case kernel crush
    if (fetched_code > caps_lock_press) {   
        send_eoi(Keyboard_IRQ_NUM);
        return;
    }
    
    
    char ascii_to_print = scancode_table[fetched_code];             // fetch the to print ascii code
#define ISALPHA(c) ((c <= 'z') && (c >= 'a'))||((c <= 'Z') && (c >= 'A'))
    if (ISALPHA(ascii_to_print)) {
		// the visible key is a lowercase letter
		// use lowercase IFF only one of shift and caps is pressed
        if ((shift_pressed ^ caps_status)) {
            ascii_to_print = upcase_scancode_table[fetched_code];
        }
    } else {
		// the visible key is not a lowercase letter
		// use lowercase IFF shift is pressed
        if ((shift_pressed)) {
            ascii_to_print = upcase_scancode_table[fetched_code];
        }
    }
#undef ISALPHA

    // control l is pressed, clear the screen, and reset cursor
    if (ctrl_status == 1 && fetched_code == l_press_scancode) {    
        clear_reset_cursor();
        printf_original("%s", enter_buffer[cur_terminal]);                         // reprint what I have entered currentlt
        send_eoi(Keyboard_IRQ_NUM);
        return;
    }

    if (ctrl_status == 1 && fetched_code == c_press_scancode) {
        send_eoi(Keyboard_IRQ_NUM);
        halt_terminal = cur_terminal;
        // halt(0);
        // actually won't reach here
        return;
    }

    // extra credit: networking
    if (ctrl_status == 1 && fetched_code == n_press_scancode) {
        send_eoi(Keyboard_IRQ_NUM);
        uint8_t googleip[4] = {66, 220, 146, 94};
        uint16_t src_port = 33333;  // randomly picked one
        uint16_t dst_port = 52341;  // randomly picked one
        uint8_t data[5];
        data[0] = 'H';
        data[1] = 'e';
        data[2] = 'l';
        data[3] = 'l';
        data[4] = 'o';
        uint32_t udp_length = 5;
        disable_irq(0);
        udp_send(googleip, src_port, dst_port, data, udp_length);
        enable_irq(0);
        return;
    }

    // extra credit: networking information
    if (ctrl_status == 1 && fetched_code == i_press_scancode) {
        send_eoi(Keyboard_IRQ_NUM);
        printf_original("Self_MAC_ADDRESS: %x:%x:%x:%x:%x:%x\n", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
        pci_brute_force_enumerate();
        return;
    }

    // piano
    if (ctrl_status == 1 && fetched_code == p_press_scancode) {
        send_eoi(Keyboard_IRQ_NUM);
        if (piano_on == 0)
        {
            piano_on = 1;
        }
        else
        {
            piano_on = 0;
        }
        return;
    }

    // extra credit: recursive halt
    if (ctrl_status == 1 && fetched_code == q_press_scancode) {
        send_eoi(Keyboard_IRQ_NUM);
        recursive_halt_terminal = cur_terminal;
        recursive_halt_counter = terminals[cur_terminal].num_process - 1;
        // recursive_halt_counter = 3;
        return;
    }

    // extra credit: show all terminals
    if (ctrl_status == 1 && fetched_code == 0x2F) { // 'v'
        send_eoi(Keyboard_IRQ_NUM);
        int t_id;
        for (t_id = 0; t_id < NUMBER_OF_TERMINAL; ++t_id) {
            gui_terminal[t_id].visible = 1;
        }
        return;
    }

    // if the fetched ascii code is not a functional key
    if (ascii_to_print != 0x00) {
        // extra credit: piano!
        if (piano_on)
        {
            piano(ascii_to_print);
        }
        

        if (buffer_c[cur_terminal] <= char_hold_capacity) {           // if we have not reached the must end number 128
            enter_buffer[cur_terminal][buffer_c[cur_terminal]] = ascii_to_print;
            putc_original(ascii_to_print);                       // no need for additional identation because putc adds the screen_x and screen_y
            buffer_c[cur_terminal]++;
        } else if (buffer_c[cur_terminal] == max_buffer_index) {      // if we reach 127 in index, manually add \n
            enter_buffer[cur_terminal][buffer_c[cur_terminal]] = '\n';
        }
        // do nothing if ew already have 128 chars in the buffer 
    }

    send_eoi(Keyboard_IRQ_NUM); 
    sti();
    return;
}


// extra credit: history
void init_whether() {
	whether_record[0] = 1;
	whether_record[1] = 1;
	whether_record[2] = 1;
}

/*
 * record_buf()
 *   DESCRIPTION: This function increments the current enter buffer into record history by 1
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes the history record structure
 */
void record_buf() {
	int j;
	int i;
	for (i = 0; i <= max_buffer_index; i++) {
		for (j = REM - 1; j >= 1; j--) {
			prev_one_read[j][cur_terminal][i] = prev_one_read[j - 1][cur_terminal][i];
		}
		prev_one_read[0][cur_terminal][i] = enter_buffer[cur_terminal][i];
		
	}
	for (j = REM - 1; j >= 1; j--) {
		buffer_prev[j][cur_terminal] = buffer_prev[j-1][cur_terminal];
	}
	
	buffer_prev[0][cur_terminal] = buffer_c[cur_terminal];
}

/*
 * copy_into_enter()
 *   DESCRIPTION: This function copies the comment specified by depth into the current enter buffer
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes enter buffer
 */
void copy_into_enter() {
	int i;
	for (i = 0; i <= max_buffer_index; i++) {
		
		enter_buffer[cur_terminal][i] = prev_one_read[depth][cur_terminal][i];
		
	}
}

int if_head_match(char * a, char * b) {
    int len_a = strlen(a);
    int len_b = strlen(b);
    if (len_a > len_b) {
        return 0;
    }
    int i;
    int ret = 1;
    for (i = 0; i < len_a; i++) {
        if (a[i] != b[i]) {
            ret = 0;
            break;
        }
        
    }
    return ret;
}



