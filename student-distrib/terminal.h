#include "Device/keyboard_handler.h"
#include "lib.h"
// terminal read function
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);

// terminal write function
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);

// terminal open function
int32_t terminal_open (const uint8_t* filename);

// terminal close function
int32_t terminal_close (int32_t fd);


