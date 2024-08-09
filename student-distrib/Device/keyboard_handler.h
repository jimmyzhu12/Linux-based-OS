// defined as a handler to the keyboard input

// function 1 hande_interrupt

#define REM 8
#define NUM_Terminal 3
#define KB_read_port 0x60
#define KB_PIC_IRQ_NUM   1
// int buffer_c;           // current number of characters
// char enter_buffer[128]; // this is the buffer to hold what we want to write into the screen
int buffer_c[NUM_Terminal];               // modified in cp5: 3 terminals
char enter_buffer[NUM_Terminal][128];    // modified in cp5: 3 terminals
char read_buffer[NUM_Terminal][128];       // modified in cp5: 3 terminals
int enter_flag;             // detect if we press enter
int piano_on;
char file_names[16][50];
// extra credit: history
char prev_one_read[REM][3][128];     // the first prev buffer
int buffer_prev[REM][3];
extern void record_buf();
extern void copy_into_enter();

/* enable the keyboard interrupt */
extern void keyboard_init();

/* handle keyboard interrupt, reading the scancode */
extern void handle_kb_interrupt();

/* copy from the buffer I am writing to the one to be sent to kernel */
extern void copy_buffers();

/* make contents of both buffer all NULL */
extern void init_buffer();
extern void init_whether();
extern void init_enter();
extern int if_head_match(char * a, char * b);



