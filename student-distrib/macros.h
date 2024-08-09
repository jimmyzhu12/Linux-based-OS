#ifndef BASE_MACROS_H
#define BASE_MACROS_H

#define NULL 0

#define VIDEO			0x80000
#define SCREEN_SIZE 	4096    // screen_x * screen_y * 2
#define NUM_COLS		80
#define NUM_ROWS		25

#define BORDER			15
#define TOP_BAR_HEIGHT	20

#define TEXT			0		/* Compatible with VGAlib v1.2 */
#define G640x480x32K	17		/* <- mode we choose! */
#define G800x600x32K	20		/* <- mode we choose! */
#define G1024x768x32K	23		/* <- mode we choose! */

// system-scale variables
#define NUMBER_OF_TERMINAL	3
#define MAX_PROCESS			6

#endif /* BASE_MACROS_H */
