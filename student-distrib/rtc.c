#include "rtc.h"
#include "i8259.h"
#include "lib.h"
#include "types.h"

// Reference from https://wiki.osdev.org/RTC
// https://wiki.osdev.org/CMOS
// and https://wiki.osdev.org/NMI
// CMOS registers
#define CMOS_SECONDS 0x00
#define CMOS_MINUTES 0x02
#define CMOS_HOURS   0x04
#define CMOS_WEEKDAY 0x06   // 1-7, Sunday = 1
#define CMOS_DAY_OF_MONTH 0x07  // 1-31
#define CMOS_MONTH   0x08   // 1-12
#define CMOS_YEAR    0x09   // 0-99
#define CMOS_CENTURY 0x32   //19-20 maybe
#define CMOS_A       0x8A
#define CMOS_B       0x8B
#define CMOS_C       0x0C

// extra credit
#define BIT_3  0x80
real_time_t real_time;
// extra credit end

#define RTC_REG_PORT    0x70
#define RTC_DATA_PORT   0x71
#define RTC_IRQNUM      8
#define DEFAULT_FREQ    6  // 1024Hz
#define TEST_FREQ       15 // 2Hz

#define MIN_FREQ        2  // 2Hz
#define MAX_FREQ        1024 // according to the document and demo video, we should limit the max freq to be less than 1024


// flag for new rtc interrupt: if this number changed, an interrupt has happened
volatile int32_t rtc_int_numbers = 0;



/* indicates whether rtc test mode is enabled (0 is enabled) */
enum rtc_test_mode {
	RTC_TEST_ON = 0,
	RTC_TEST_OFF = 1
} rtc_mode;

/* 
 *  rtc_set_reg
 *  DESCRIPTION: set an RTC register
 *  INPUTS: reg - the register that is going to be set
  *         value - the value that is going to be set into the register
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: set the value in the given register
 */
void rtc_set_reg(int8_t reg, int8_t value)
{
    cli();		        // important that no interrupts happen (perform a CLI)
    outb(reg, RTC_REG_PORT);	// select Status Register A, and disable NMI (by setting the 0x80 bit)
    outb(value, RTC_DATA_PORT);	// write to CMOS/RTC RAM
    sti();		        // (perform an STI) and reenable NMI if you wish
}

/* 
 *  rtc_get_reg
 *  DESCRIPTION: get the value of an RTC register
 *  INPUTS: reg - the register whose value is going to be get
 *  OUTPUTS: int8_t - the value in the assigned register
 *  RETURN VALUE: int8_t - the value in the assigned register
 *  SIDE EFFECTS: none
 */
int8_t rtc_get_reg(int8_t reg)
{
    outb(reg, RTC_REG_PORT);
    return inb(RTC_DATA_PORT);
}

/* 
 *  rtc_turn_on_irq8
 *  DESCRIPTION: turn on the rtc
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: turn on the rtc
 */
void rtc_turn_on_irq8()
{
    cli();
    int8_t data;
    data = rtc_get_reg(CMOS_B);
    rtc_set_reg(CMOS_B, data | 0x40);   /* 0x40 is a bit mask */
    sti();
}

/* 
 *  rtc_init
 *  DESCRIPTION: initialize the rtc
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: initialize the rtc, disable the test mode
 */
void rtc_init()
{
    cli();
	rtc_test_disable();
    rtc_turn_on_irq8();
    // pic unmask
    enable_irq(RTC_IRQNUM);
    rtc_get_reg(CMOS_C);    // allow next irq
    change_freq_by_rate(TEST_FREQ);
    rtc_int_numbers = 0;
    sti();
}

/* 
 *  change_freq
 *  DESCRIPTION: change the frequency of rtc
 *  INPUTS: freq_rate - the frequency rate of rtc
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: change the frequency of rtc
 */
// freq_rate must be above 2 and not over 15. 6 is for 1024
// int32_t freq = 32678 >> (freq_rate - 1);
void change_freq_by_rate(int8_t freq_rate)
{
    int8_t rate = freq_rate & 0x0F;
    cli();
    int8_t prev = rtc_get_reg(CMOS_A);
    rtc_set_reg(CMOS_A, (prev & 0xF0) | rate);
    sti();
}

/* 
 *  set_freq - Added in CP2
 *  DESCRIPTION: change the frequency of rtc, according to the given frequency number
 *  INPUTS: freq - the frequency of rtc that is going to be set
 *  OUTPUTS: none
 *  RETURN VALUE: int32_t. 0 for success, -1 for fail
 *  SIDE EFFECTS: change the frequency of rtc
 */
// int32_t freq = 32678 >> (freq_rate - 1);
// freq_rate must be above 2 and not over 15. 15 is for 2. 6 is for 1024. 3 is for 8192. 

int32_t set_freq(uint32_t freq)
{
    // check the input validity
    // first check if it's within the closed interval between 2 and 1024
    if (freq < MIN_FREQ)
        return -1;
    if (freq > MAX_FREQ)
        return -1;
    // one-line formula to judge whether a number is power of 2 according to https://www.ritambhara.in/check-if-number-is-a-power-of-2/
    if ((freq & (freq - 1)) != 0)
        return -1;
    // calculate the corresponding rate
    int8_t log = 0;
    while (freq >>= 1)
    {
        log += 1;
    }
    int8_t rate = 16 - log;
    // set the freq rate using previous function
    change_freq_by_rate(rate);
    return 0;
}

/* 
 *  rtc_handler
 *  DESCRIPTION: handle rtc interrupt
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: do test_interrupts()
 */
void rtc_handler()
{
    // should I do this here?
	if (rtc_mode == RTC_TEST_ON) {
		test_interrupts();
	}
    rtc_int_numbers += 1;
    // allow next irq
    rtc_get_reg(CMOS_C);
    send_eoi(RTC_IRQNUM);
    sti();
}

/* 
 *  NMI_enable
 *  DESCRIPTION: enable non-maskable interrupts
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: values in rtc registers might be changed
 */
void NMI_enable()
{
    outb(RTC_REG_PORT, inb(RTC_REG_PORT) & 0x7F);   // 7F is used to set the highest bit to 0
    inb(RTC_DATA_PORT);
}

/* 
 *  NMI_disable
 *  DESCRIPTION: disable non-maskable interrupts
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: values in rtc registers might be changed
 */
void NMI_disable()
{
    outb(RTC_REG_PORT, inb(RTC_REG_PORT) | 0x80);   // 80 is used to set the highest bit to 1
    inb(RTC_DATA_PORT);
}


// RTC drivers

/* 
 *  rtc_open
 *  DESCRIPTION: provides access to the file system related to rtc part.
 *  INPUTS: filename - not used at all
 *  OUTPUTS: none
 *  RETURN VALUE: 0 for success. However always return 0
 *  SIDE EFFECTS: the frequency of rtc will be set to 2Hz
 */
int32_t rtc_open(const uint8_t* filename)
{
    // initialized RTC frequency to 2Hz, return 0, as given in the discussion slides
    set_freq(2);    // impossible to fail because of the given number 2
    return 0;
}

/* 
 *  rtc_close
 *  DESCRIPTION: close the specified file descriptor, which is the rtc
 *  INPUTS: fd - not used at all
 *  OUTPUTS: none
 *  RETURN VALUE: 0 for success. However always return 0
 *  SIDE EFFECTS: none
 */
int32_t rtc_close(int32_t fd)
{
    // do nothing unless RTC is vertualized
    return 0;
}

/* 
 *  rtc_read
 *  DESCRIPTION: should always return 0, but only after an interrupt has occurred
                 Not for reading the RTC frequency
 *  INPUTS: fd - file descriptor. not used at all
            buf - buffer. not used at all
            nbytes - bytes in the buffer. not used at all
 *  OUTPUTS: none
 *  RETURN VALUE: 0 for success. However always return 0
 *  SIDE EFFECTS: none
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes)
{
    // block until the next interrupt.
    int32_t old_int_number = rtc_int_numbers;
    // do the block
    while(rtc_int_numbers == old_int_number);
    // if next interrupt happens, rtc_int_numbers will increase, which will break the while loop
    return 0;
}

/* 
 *  rtc_write
 *  DESCRIPTION: should always accept only a 4-byte integer specifying the interrupt rate in Hz,
                and should set the rate of periodic interrupts accordingly (from document)
 *  INPUTS: fd - file descriptor. not used at all
            buf - buffer containing the frequency that is going to be set
            nbytes - bytes in the buffer. This should always be 4. Need to check
 *  OUTPUTS: none
 *  RETURN VALUE: 0 for success. -1 for failure.
 *  SIDE EFFECTS: none
 */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes)
{
    // check the validity of inputs first
    if (buf == NULL)
        return -1;
    if (nbytes != 4)
        return -1;
    // get the new frequency that is going to be set
    int32_t freq = *(int32_t*) buf;
    // set the frequency. return according to whether the setting succeeds.
    if (set_freq(freq) == 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/*
 * rtc_test_enable/rtc_test_disable
 *   DESCRIPTION: Enables/Disables rtc test mode
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: rtc test mode is enabled/disabled
 */
void rtc_test_enable() {
	rtc_mode = RTC_TEST_ON;
}
void rtc_test_disable() {
	rtc_mode = RTC_TEST_OFF;
}



// extra credit:
// Reference from https://wiki.osdev.org/CMOS#Getting_Current_Date_and_Time_from_RTC
/*
 * get_update_in_progress_flag
 *   DESCRIPTION: get the flag which signals rtc update in progress
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int get_update_in_progress_flag()
{
    outb(CMOS_A, RTC_REG_PORT);
    return (inb(RTC_DATA_PORT) & BIT_3);
}

/*
 * read_real_time_from_rtc
 *   DESCRIPTION: update current time
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: a pointer
 *   SIDE EFFECTS: none
 */
real_time_t* update_real_time_from_rtc()
{

    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint8_t year;
    uint8_t registerB;

    second = rtc_get_reg(CMOS_SECONDS);
    minute = rtc_get_reg(CMOS_MINUTES);
    hour = rtc_get_reg(CMOS_HOURS);
    day = rtc_get_reg(CMOS_DAY_OF_MONTH);
    month = rtc_get_reg(CMOS_MONTH);
    year = rtc_get_reg(CMOS_YEAR);
    registerB = rtc_get_reg(CMOS_B);

    while(get_update_in_progress_flag())
    {
        second = rtc_get_reg(CMOS_SECONDS);
        minute = rtc_get_reg(CMOS_MINUTES);
        hour = rtc_get_reg(CMOS_HOURS);
        day = rtc_get_reg(CMOS_DAY_OF_MONTH);
        month = rtc_get_reg(CMOS_MONTH);
        year = rtc_get_reg(CMOS_YEAR);
        registerB = rtc_get_reg(CMOS_B);
    }
     // Convert BCD to binary values if necessary, as mentioned on osdev
 
    if (!(registerB & 0x04)) {
        second = (second & 0x0F) + ((second / 16) * 10);
        minute = (minute & 0x0F) + ((minute / 16) * 10);
        hour = ( (hour & 0x0F) + (((hour & 0x70) / 16) * 10) ) | (hour & 0x80);
        day = (day & 0x0F) + ((day / 16) * 10);
        month = (month & 0x0F) + ((month / 16) * 10);
        year = (year & 0x0F) + ((year / 16) * 10);
    }
 
    // Convert 12 hour clock to 24 hour clock if necessary, as mentioned on osdev

    if (!(registerB & 0x02) && (hour & 0x80)) {
        hour = ((hour & 0x7F) + 12) % 24;
    }


    // store into the structure
    real_time.second = second;
    real_time.minute = minute;
    real_time.hour = (hour + 19) % 24;
    real_time.day = day;
    real_time.month = month;
    real_time.year = year;

	return &real_time;
}
