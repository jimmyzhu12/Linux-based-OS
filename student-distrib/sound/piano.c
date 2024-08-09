        
#include "piano.h"        
        
void piano(char ascii_to_print)
{
    switch (ascii_to_print)
    {
        // lowest row: zxcvbnm,./
        case 'z':
            beep(1);
            /* code */
            break;
        case 'x':
            beep(2);
            break;
        case 'c':
            beep(3);
            break;
        case 'v':
            beep(4);
            break;
        case 'b':
            beep(5);
            break;
        case 'n':
            beep(6);
            break;
        case 'm':
            beep(7);
            break;
        case ',':
            beep(8);
            break;
        case '.':
            beep(9);
            break;
        case '/':
            beep(10);
            break;

        // second row: sd ghj l;
        case 's':
            beep(11);
            break;
        case 'd':
            beep(12);
            break;
        case 'g':
            beep(13);
            break;
        case 'h':
            beep(14);
            break;
        case 'j':
            beep(15);
            break;
        case 'l':
            beep(16);
            break;
        case ';':
            beep(17);
            break;

        // Third row: qwertyuiop
        case 'q':
            beep(18);
            /* code */
            break;
        case 'w':
            beep(19);
            break;
        case 'e':
            beep(20);
            break;
        case 'r':
            beep(21);
            break;
        case 't':
            beep(22);
            break;
        case 'y':
            beep(23);
            break;
        case 'u':
            beep(24);
            break;
        case 'i':
            beep(25);
            break;
        case 'o':
            beep(26);
            break;
        case 'p':
            beep(27);
            break;

        // fourth row: 23 567 90;
        case '2':
            beep(28);
            break;
        case '3':
            beep(29);
            break;
        case '5':
            beep(30);
            break;
        case '6':
            beep(31);
            break;
        case '7':
            beep(32);
            break;
        case '9':
            beep(33);
            break;
        case '0':
            beep(34);
            break;

        default:
            break;
    }

}

