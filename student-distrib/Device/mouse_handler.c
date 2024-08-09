


// reference:
// https://wiki.osdev.org/Mouse_Input#USB_Mouse
// osdev chart and example 
#include "mouse_handler.h"
#include "../gui/gui.h"
#include "../gui/fonts.h"
#include "../gui/gui_obj_data.h"
#include "../gui/vga.h"
#include "../cp5.h"

int mouseX;
int mouseY;
int mouseX_maximum;
int mouseY_maximum;
int mouse_dpi_control;
int mouse_left_pressed;			/* pressed iff != 0 */

#define mouse_IRQ_NUM 12    // subject to change
#define ACK_MESSAGE 0xFA
#define READ_DATA_PORT 0x60
#define READ_DESCRIPTION 0x64
#define disable_bit_5 0xDF
#define Get_Compaq_Status 0x20
#define Disable_Packet_Streaming 0xF5
#define Set_Defaults 0xF6
#define Enable_Packet_Streaming	0xF4
#define make_negative 0xFFFFFF00
#define Enable_Auxiliary 0xA8
#define left_bottom 0x1
#define Overflow_1 0x80
#define Overflow_2 0x40
#define X_sign_bit 0x10
#define Y_sign_bit 0x20

int set_mouse_range(int x, int y) {
	unsigned int IF;
	cli_and_save(IF);
	{
		mouseX_maximum = x;
		mouseY_maximum = y;
		if (mouseX > mouseX_maximum) mouseX = mouseX_maximum;
		if (mouseY > mouseY_maximum) mouseY = mouseY_maximum;
	}
	restore_flags(IF);
	return 1;
}
int get_mouse_pos(int* x, int* y) {
	unsigned int IF;
	cli_and_save(IF);
	{
		*x = mouseX;
		*y = mouseY;
	}
	restore_flags(IF);
	return 1;
}


//Sending a command or data byte to the mouse (to port 0x60) must be preceded by sending a 0xD4 byte to port 0x64 (with appropriate waits on port 0x64, bit 1, 
//before sending each output byte). Note: this 0xD4 byte does not generate any ACK, from either the keyboard or mouse.
void send_mouse_info(char write) {
	mouse_wait(0);
	//Sending a command or data byte to the mouse (to port 0x60) must be preceded by sending a 0xD4 byte to port 0x64
	outb(0xD4,READ_DESCRIPTION);
	mouse_wait(0);
	outb(write, READ_DATA_PORT);
}

// It is required to wait until the mouse sends back the 0xFA acknowledgement byte after each command or data byte before sending the next byte

void wait_acknowledge() {
	volatile int tmp = 0x00;
	while (1) {
		mouse_wait(0);
		tmp = inb(READ_DATA_PORT);
		if (tmp == ACK_MESSAGE) {
			break;
		}
	}
}

/*
 * mouse_init()
 *   DESCRIPTION: This function initliaze the mouse irq and reset the mouse position
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: mouse irq turned on
 */
void mouse_init(){
	// initize the maximum size screen to text mode
	set_mouse_range(79, 24);
	/* 13 is an EViL NUMBER! -- Howie 05/02/2022 */
	/*  8 is a great NUMBER! -- Howie 05/03/2022 */
	/*  6 is a great NUMBER as well! -- Howie 05/05/2022 */
	mouse_dpi_control = 6;   // this is the normalization constant for dpi
	char status;
	mouse_wait(0);
	outb(Get_Compaq_Status, READ_DESCRIPTION);
	mouse_wait(0);
	status = inb(READ_DATA_PORT);                  // read the status, and set the mouse enable
	status = status | 2;                 // enabling the IRQ 12
	status = status & disable_bit_5;              // disabling bit number 5
	mouse_wait(0);
	outb(READ_DATA_PORT,READ_DESCRIPTION);
	mouse_wait(0);
	outb(status, READ_DATA_PORT);
	send_mouse_info(Disable_Packet_Streaming);                // mask the automatic packets
	wait_acknowledge();
	send_mouse_info(Set_Defaults);
	wait_acknowledge();
	send_mouse_info(Enable_Packet_Streaming);                // reenabling the automatic packets
	wait_acknowledge();
	mouse_wait(0);
	outb(Enable_Auxiliary, READ_DESCRIPTION);                    // do the Enable Auxiliary Device command
	
	enable_irq(mouse_IRQ_NUM);
	mouseX = 40;                         // reset mouesX and mouseY to the original position, subject to change ( should be the middle point of the screen)
	mouseY = 12;
	mouse_left_pressed = 0;              // initialized the pressed state to not pressed
	printf("init mouse");
}
// direct quote from osdev:
// All output to port 0x60 or 0x64 must be preceded by waiting for bit 1 (value=2) of port 0x64 to become clear.
// Similarly, bytes cannot be read from port 0x60 until bit 0 (value=1) of port 0x64 is set. See PS2 Keyboard for further details.
// write must wait for bit 1 of 0x64
// read must wait for clearence of bit 2 of 0x64
/*
 * mouse_wait()
 *   DESCRIPTION: This function waits until the mouse is ready to receive or send packet
 *   INPUTS: int read_or_write 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: mouse irq turned on
 */
void mouse_wait(int read_or_write) {
	volatile int time = 100000;
	switch (read_or_write) {
		case 1:   
			while(time--) {
				if (0x1 & inb(READ_DESCRIPTION)) {    // this is for read from 0x60
					return;
				}
			}
			break;
		case 0:
			while(time--) {
				if (!(0x2 & inb(READ_DESCRIPTION))) { // this is for write to 0x60 and 0x64
					return;
				}
			}
			break;
	}
	return;
}

static inline int mouse_in_range(int x, int y, int w, int h) {
	return (mouseX >= x && mouseY >= y && mouseX < x + w && mouseY < y + h);
}

/*
 * handle_mo_interrupt()
 *   DESCRIPTION: This function handles mouse interrupt, reset the cursor
 *   INPUTS: none
 *   OUTPUTS: update the cursor with the mouse 
 *   RETURN VALUE: none
 *   SIDE EFFECTS: prints to screen/ reset cursor
 */

static int long_pressed_ter;
void handle_mo_interrupt() {
	
	cli();
	// direct quote from osdev:
	//  it is necessary to read a byte from port 0x64. 
	//In that byte from port 0x64, bit number 0 (value=1) indicates that a byte is available to be read on port 0x60. 
	//An additional bit, bit number 5 (value=0x20), indicates that this next byte came from the mouse,
	// THIS LINE IS MAKE SURE THE DATA MESSAGE IS FROM MOUSE, BECAUSE OTHER DEVICES ALSO ON THIS BUS
	if (!(0x20 & inb(READ_DESCRIPTION))) {    // if the message is not from the mouse, do not do anything on this one.
		send_eoi(mouse_IRQ_NUM);
		return;
	}

	uint8_t message = inb(READ_DATA_PORT);

	// wait for mouse data is available
	mouse_wait(1);
	int32_t x_move = inb(READ_DATA_PORT);
	// wait for the third packet has arrived
	mouse_wait(1);
	int32_t y_move = inb(READ_DATA_PORT);


	if ((message & Overflow_1) || (message & Overflow_2)) {
		send_eoi(mouse_IRQ_NUM);
		return;
	}

	if (message & X_sign_bit) {
		x_move = x_move | make_negative;
	}
	if (message & Y_sign_bit) {
		y_move = y_move | make_negative;
	}
	x_move /= mouse_dpi_control;               // normalizing the mouse speed using the pre set factor
	y_move /= mouse_dpi_control;               // normalizing the mouse speed using the pre set factor
	mouseX += x_move;
	 // I have to reverse for my Y packet, because down means positive in this way
	mouseY -= y_move;    
	// confined it into the screen screen x most 79, screen_y most 24
	if (mouseX < 0) {
		x_move -= mouseX;
		mouseX = 0;
	}
	if (mouseY < 0) {
		y_move += mouseY;
		mouseY = 0; 
	}
	if (mouseX > mouseX_maximum) {
		x_move -= mouseX - mouseX_maximum;
		mouseX = mouseX_maximum;
	}
	if (mouseY > mouseY_maximum) {
		y_move += mouseY - mouseY_maximum;
		mouseY = mouseY_maximum;
	}
	update_cursor(mouseX, mouseY);    

	if (message & left_bottom) {
		if (mouse_left_pressed == 0) {
			// if previously mouse is just pressed, update priority and record last mouse
			int traverse_t;
			int g;
			int p_remember[NUMBER_OF_TERMINAL];
			for (g = 0; g < NUMBER_OF_TERMINAL; g++) {
				p_remember[gui_terminal[g].priority] = g;
			}
			for (traverse_t = NUMBER_OF_TERMINAL - 1; traverse_t >= 0 ; traverse_t--) {
				int tmp_ter = p_remember[traverse_t];
				if (gui_terminal[tmp_ter].visible != 0 &&
					mouse_in_range(gui_terminal[tmp_ter].x, gui_terminal[tmp_ter].y,
						TERMINAL_WIDTH, TERMINAL_HEIGHT))
				{
					int p_traverse;
					// p_remember[traverse_t] is the terminal
					for (p_traverse = traverse_t + 1; p_traverse < NUMBER_OF_TERMINAL; p_traverse++) {
						gui_terminal[p_remember[p_traverse]].priority--;
					}
					gui_terminal[p_remember[traverse_t]].priority = NUMBER_OF_TERMINAL - 1;
					terminal_switch(p_remember[traverse_t]);
					// is the mouse in the range of status bar?
					if (mouseY < gui_terminal[tmp_ter].y + UPPER_FRAME_HEIGHT) {
						long_pressed_ter = p_remember[traverse_t];
					} else {
						long_pressed_ter = -1;
					}
					// jump out of loop
					traverse_t = -1;
				}
			}
		}
		if (mouse_left_pressed == 1) {
			if (long_pressed_ter >= 0) {
				gui_terminal[long_pressed_ter].x += x_move;
				gui_terminal[long_pressed_ter].y -= y_move;
				if (gui_terminal[long_pressed_ter].x + cur_vga_info->xdim < BORDER) {
					gui_terminal[long_pressed_ter].x = BORDER - cur_vga_info->xdim ;
				} else if (gui_terminal[long_pressed_ter].x > cur_vga_info->xdim - BORDER) {
					gui_terminal[long_pressed_ter].x = cur_vga_info->xdim - BORDER;
				}
				if (gui_terminal[long_pressed_ter].y < TOP_BAR_HEIGHT) {
					gui_terminal[long_pressed_ter].y = TOP_BAR_HEIGHT;
				} else if (gui_terminal[long_pressed_ter].y > cur_vga_info->ydim - BORDER) {
					gui_terminal[long_pressed_ter].y = cur_vga_info->ydim - BORDER;
				}
			}
		}
		mouse_left_pressed = 1;
	} else {   // if the click is released
		if (gui_terminal[long_pressed_ter].visible != 0) {
			if (mouse_in_range(gui_terminal[long_pressed_ter].x + BT_R_X_offset,
				               gui_terminal[long_pressed_ter].y + BT_Y_offset,
				               BOTTOM_WIDTH, BOTTOM_WIDTH))
			{
				recursive_halt_terminal = long_pressed_ter;
        		recursive_halt_counter = terminals[long_pressed_ter].num_process - 1;
				gui_terminal[long_pressed_ter].visible = 0;
			} else if (mouse_in_range(gui_terminal[long_pressed_ter].x + BT_Y_X_offset,
				                      gui_terminal[long_pressed_ter].y + BT_Y_offset,
				                      BOTTOM_WIDTH, BOTTOM_WIDTH)){
				gui_terminal[long_pressed_ter].visible = 0;
			}
		}
		long_pressed_ter = -1;
		mouse_left_pressed = 0;
	} 
	send_eoi(mouse_IRQ_NUM);
	sti();
}


