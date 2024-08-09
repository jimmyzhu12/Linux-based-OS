/* Header file for rtc
 *
 */
#ifndef RTC_H
#define RTC_H
#include "types.h"

// Reference from https://wiki.osdev.org/RTC
// https://wiki.osdev.org/CMOS
// and https://wiki.osdev.org/NMI
extern void rtc_init();

int8_t rtc_get_reg(int8_t reg);
void rtc_set_reg(int8_t reg, int8_t value);
void rtc_turn_on_irq8();
void change_freq_by_rate(int8_t freq_rate);
int32_t set_freq(uint32_t freq);
void rtc_handler();

// NMI (non-maskable interrupt) functions
void NMI_enable();
void NMI_disable();

// rtc drivers. See Appendix B for more detail
extern int32_t rtc_open(const uint8_t* filename);
extern int32_t rtc_close(int32_t fd);
extern int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);
extern int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);

// enable rtc test mode
extern void rtc_test_enable();

// disable rtc test mode
extern void rtc_test_disable();


// extra credit: real time
// reference from https://wiki.osdev.org/CMOS#Getting_Current_Date_and_Time_from_RTC
typedef struct real_time_t
{
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint8_t year;
} real_time_t;

int get_update_in_progress_flag();
real_time_t* update_real_time_from_rtc();
// void rtc_set_page();

#endif
