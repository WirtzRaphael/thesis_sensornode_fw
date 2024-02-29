#include "low_power_operations.h"
#include "pico/sleep.h"
#include "pico/stdlib.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "hardware/clocks.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"

bool awake;
uint scb_orig;
uint clock0_orig;
uint clock1_orig;

bool powerstate_awake;

// RTC - Low Power
//reset the clock each time to the value below
datetime_t t = {
        .year  = 2020,
        .month = 06,
        .day   = 05,
        .dotw  = 5, // 0 is Sunday, so 5 is Friday
        .hour  = 15,
        .min   = 45,
        .sec   = 00
};

static void low_power_sleep_callback(void) {
    printf("RTC woke us up\n");
    uart_default_tx_wait_blocking();
    awake = true;
    return;
}

static void low_power_rtc_sleep(int8_t minute_to_sleep_to, int8_t second_to_sleep_to, bool *state) {

    datetime_t t_alarm = {
            .year  = 2020,
            .month = 06,
            .day   = 05,
            .dotw  = 5, // 0 is Sunday, so 5 is Friday
            .hour  = 15,
            .min   = minute_to_sleep_to,
            .sec   = second_to_sleep_to
    };
    

    printf("Going to sleep.......\n");
    uart_default_tx_wait_blocking();

    sleep_goto_sleep_until(&t_alarm, &low_power_sleep_callback);
    //*state = awake;
    *state = true;
}

//void low_power_recover_from_sleep(uint scb_orig, uint clock0_orig, uint clock1_orig){
static void low_power_recover_from_sleep(void){

    //Re-enable ring Oscillator control
    rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);

    //reset procs back to default
    scb_hw->scr = scb_orig;
    clocks_hw->sleep_en0 = clock0_orig;
    clocks_hw->sleep_en1 = clock1_orig;

    //reset clocks
    clocks_init();
    stdio_init_all();

    return;
}

void low_power_init(void) {
    powerstate_awake = true;

    //save values to recover from sleep
    scb_orig = scb_hw->scr;
    clock0_orig = clocks_hw->sleep_en0;
    clock1_orig = clocks_hw->sleep_en1;
}

void low_power_sleep(void) {
        /* debug
        clock_measure_freqs();
        uart_default_tx_wait_blocking();
        */

        // sleep from xosc
        sleep_run_from_xosc();

        // fixme : possible conflict with global rtc reset
        // fixme : don't reset to default value, but to the value of the rtc
        // reset real time clock to a value
        rtc_set_datetime(&t);

        powerstate_awake = false;
        low_power_rtc_sleep(45, 10, &powerstate_awake);
        // return from sleep here and powerstate_awake should be true
        while (!powerstate_awake) { 
            /* Sleeping */
            uart_default_tx_wait_blocking();
        }

        // reset processor and clocks back to defaults
        low_power_recover_from_sleep();;

        /* debug
        //clocks should be restored
        clock_measure_freqs();
        */
}

void clock_measure_freqs(void) {
    uint f_pll_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_SYS_CLKSRC_PRIMARY);
    uint f_pll_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_PLL_USB_CLKSRC_PRIMARY);
    uint f_rosc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC);
    uint f_clk_sys = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS);
    uint f_clk_peri = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI);
    uint f_clk_usb = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB);
    uint f_clk_adc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC);
    uint f_clk_rtc = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_RTC);

    printf("pll_sys  = %dkHz\n", f_pll_sys);
    printf("pll_usb  = %dkHz\n", f_pll_usb);
    printf("rosc     = %dkHz\n", f_rosc);
    printf("clk_sys  = %dkHz\n", f_clk_sys);
    printf("clk_peri = %dkHz\n", f_clk_peri);
    printf("clk_usb  = %dkHz\n", f_clk_usb);
    printf("clk_adc  = %dkHz\n", f_clk_adc);
    printf("clk_rtc  = %dkHz\n", f_clk_rtc);
    uart_default_tx_wait_blocking();
    // Can't measure clk_ref / xosc as it is the ref
}
