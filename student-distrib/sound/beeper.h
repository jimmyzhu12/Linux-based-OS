#ifndef _BEEPER_H
#define _BEEPER_H

// Reference from https://wiki.osdev.org/PC_Speaker
// and http://kernelx.weebly.com/pc-speaker.html
#include "../lib.h"
#include "../rtc.h"
#include "../Scheduler.h"
#define MAX_FREQUENCE  1193180
#define PIT_MODE_REG    0x43
#define BEEP_MODE   0xB6
#define CH3_D_PORT  0x42

void play_sound(uint32_t nFrequence);
void nosound();
void beep(int i);


#endif

