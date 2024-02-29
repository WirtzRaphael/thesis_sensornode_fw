#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"

// ref : rp2040 datasheet
// note : not safe when called from multiple cores
uint64_t time_operations_get_time_us(void) {
    // Reading low latches the high value
    uint32_t lo = timer_hw->timelr;
    uint32_t hi = timer_hw->timehr;
    return ((uint64_t) hi << 32u) | lo;
}

uint64_t time_operations_get_time_diff_us(uint64_t time_start, uint64_t time_end) {
    return time_end - time_start;
}

uint32_t time_operations_get_time_diff_s(uint64_t time_start, uint64_t time_end) {
    uint64_t time_diff_us = time_operations_get_time_diff_us(time_start, time_end);
    //uint64_t time_diff_s = time_diff_us / 1000000;
    return (uint32_t) time_diff_us >> 20; // Shift 20 bits to the right (equivalent to dividing by 1,048,576)
}

void time_operations_datetime_from_now(datetime_t *t, int32_t sec)
{
    datetime_t t_now;
    rtc_get_datetime(&t_now);
    t->year = t_now.year;
    t->month = t_now.month;
    t->day = t_now.day;
    t->dotw = t_now.dotw;
    t->hour = t_now.hour;
    t->min = t_now.min;
    t->sec = t_now.sec + sec;
}

void time_operations_datetime_print(datetime_t *t)
{
    /*
    unsigned char t_string [50];
    datetime_to_str(t_string, sizeof(t_string), &t_now);
    printf("%s", t_string);
    */
    printf("%d-%d-%d %d:%d:%d\n", t->year, t->month, t->day, t->hour, t->min, t->sec);
}