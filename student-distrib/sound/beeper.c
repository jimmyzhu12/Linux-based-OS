// Reference from https://wiki.osdev.org/PC_Speaker
// and http://kernelx.weebly.com/pc-speaker.html
#include "beeper.h"
#include "../Device/keyboard_handler.h"

void play_sound_init(uint32_t nFrequence)
{
    uint32_t div = MAX_FREQUENCE / nFrequence;
    outb(BEEP_MODE, PIT_MODE_REG);
    outb((uint8_t) div, CH3_D_PORT);
    outb((uint8_t) (div >> 8), CH3_D_PORT);
    //counter_max_sch = div/DIVISOR;
 	uint8_t tmp = inb(0x61);
  	if (tmp != (tmp | 3)) {
 		outb(tmp | 3, 0x61);
 	}
}

void nosound()
{
    uint8_t tmp = inb(0x61) & 0xFC;
    outb(tmp, 0x61);

}

// void beep_do() {
//     play_sound_init(220);
//     int i = 1000000;
//     while(i) {i--;}
//     nosound();
// }

// void beep_re() {
//     play_sound_init(247);
//     int i = 1000000;
//     while(i) {i--;}
//     nosound();
// }

// void beep_

void beep(int i)
{
    int freq;
    // first row
    switch (i)
    {
        case 1:
            freq = 262;
            break;
        case 2:
            freq = 294;
            break;
        case 3:
            freq = 330;
            break;
        case 4:
            freq = 349;
            break;
        case 5:
            freq = 392;
            break;
        case 6:
            freq = 440;
            break;
        case 7:
            freq = 494;
            break;
        case 8:
            freq = 523;
            break;
        case 9:
            freq = 587;
            break;
        case 10:
            freq = 660;
            break;

        // second row
        case 11:
            freq = 277;
            break;
        case 12:
            freq = 311;
            break;
        case 13:
            freq = 370;
            break;
        case 14:
            freq = 415;
            break;
        case 15:
            freq = 466;
            break;
        case 16:
            freq = 554;
            break;
        case 17:
            freq = 622;
            break;
        default:
            freq = 500;
            break;

        // third row
        case 18:
            freq = 523;
            break;
        case 19:
            freq = 587;
            break;
        case 20:
            freq = 660;
            break;
        case 21:
            freq = 698;
            break;
        case 22:
            freq = 784;
            break;
        case 23:
            freq = 880;
            break;
        case 24:
            freq = 988;
            break;
        case 25:
            freq = 1047;
            break;
        case 26:
            freq = 1175;
            break;
        case 27:
            freq = 1319;
            break;

        // fourth row: 23 567 90
        case 28:
            freq = 554;
            break;
        case 29:
            freq = 622;
            break;
        case 30:
            freq = 740;
            break;
        case 31:
            freq = 831;
            break;
        case 32:
            freq = 932;
            break;
        case 33:
            freq = 1109;
            break;
        case 34:
            freq = 1245;
            break;

    }
    play_sound_init(freq);
    int x = 900000 / 2;
    while(x) {x--;}
    nosound();
}
