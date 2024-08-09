#ifndef MOUSE_HANDLER_H
#define MOUSE_HANDLER_H

#include "../SystemCall.h"
#include "../i8259.h"
#include "../lib.h"

/* initliaze the mouse */
extern void mouse_init();

/* send mouse info */
extern void send_mouse_info(char write);
/* wait for mouse acknowledge */
extern void wait_acknowledge();
/* mouse wait helper function */
extern void mouse_wait(int a);
/* the main handle interrupt loop */
extern void handle_mo_interrupt();

#endif /* MOUSE_HANDLER_H */
