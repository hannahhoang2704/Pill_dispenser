#include <pico/stdlib.h>
#include <pico/printf.h>
#include <stdlib.h>

#include "operation.h"

// returns system time from boot
// and changes time unit according to its size
uint64_t get_time(char * time_unit) {
    uint16_t time_s = (time_us_64() >> 32) / 1000000;
    *time_unit = 's';
    if (time_s > 60) {
        if (time_s > 3600) {
            *time_unit = 'h';
            return time_s / 3600;
        } else {

            *time_unit = 'm';
        }
    }
}

// Initialize all required pins
void init_pins() {
    printf("Initializing pins...\n");
    init_switch(SW_0);
    init_pwm();
    init_opto_fork();
    init_stepper();
    init_piezo();
    printf("Pins initialized.\n\n");
    // log(PINS);
}

// Loops infinitely until switch is pressed,
// blinking LED while looping.
// Leaves LED off after press.
void blink_until_sw_pressed(uint sw) {
    // log(W8_BLINK);
    uint8_t sw_i = abs(((int)sw - 7) - 2);
    printf("Press switch #%u to calibrate...\n", sw_i);
    uint8_t loop = 0;
    while (!switch_pressed_debounced(sw)) {
        if (loop++ == 0) toggle_pwm();
        loop %= 10;
        sleep_ms(50);
    }
    printf("Switch %u pressed.\n\n", sw_i);
    put_pwm(PWM_OFF);
    // log(PRESSED);
}

// to_opto defines whether it will rotate in or out of opto-fork
// returns number of steps taken during this function
int rotate_to_event(enum opto_events flag, bool clockwise) {
    int steps = 0;
    set_opto_flag(flag, false);
    set_opto_fork_irq(true);

    while (!opto_flag_state(flag)) {
        step(clockwise);
        ++steps;
        sleep_us(SPD_REDUC_MIN);
    }
    set_opto_fork_irq(false);
    return steps;
}

// Calibrates according to number of 'slots_left' done thus far.
// 'slots_left' is to be derived from EEPROM
void calibrate() {
    // log(CALIB_START, slot_left);
    printf("Calibrating...\n");

    int opto_width;
    int full_rev_minus_opto_width;
    int full_rev_steps = 0;

    if (state.slots_left == 0) {

        // rotate clockwise into opto-fork area
        rotate_to_event(FALL, true);

        // rotate clockwise out of opto-fork area
        // and measure opto_width
        opto_width = rotate_to_event(RISE, true);

        // rotate clockwise into opto_fork area,
        // completing full revolution
        full_rev_minus_opto_width = rotate_to_event(FALL, true);
        full_rev_steps = opto_width + full_rev_minus_opto_width;

        // align the disk with the hole,
        // according to measured opto_width minus slippage
        rotate_steps(opto_width / 2 - 10);

        printf("Full revolution: %d steps\n"
               "Opto-width: %d steps\n",
               full_rev_steps, opto_width);

    } else {

        // rotate counter-clockwise into opto-fork area
        rotate_to_event(FALL, false);

        // rotate counter-clockwise out of opto-fork
        // and measure opto_width
        opto_width = rotate_to_event(RISE, false);

        // align the disk with the hole
        // according to measured opto_width minus slippage
        rotate_steps(-opto_width / 2 + 10);

        // sleep to mitigate momentum,
        // before rotating opposite direction
        sleep_ms(50);

        // then rotate according to logged
        rotate_8th(state.slots_left);
    }
    printf("Calibration finished!\n\n");
    // log(CALIB_STOP, full_rev_steps, opto_width);
}

// Puts LED on.
// Loops infinitely until switch is pressed.
// Puts LED off after switch press.
void wait_until_sw_pressed(uint sw) {
    // log(W8);
    uint8_t sw_i = abs(((int)sw - 7) - 2);
    printf("Press switch #%u to start dispensing...\n", sw_i);
    put_pwm(PWM_SOFT);
    while (!switch_pressed_debounced(sw)) {
        sleep_ms(50);
    }
    printf("Switch #%u pressed.\n\n", sw_i);
    put_pwm(PWM_OFF);
    // log(PRESSED);
}

// Dispense pills according 'slots_left',
// which is read from EEPROM on boot.
void dispense() {
    // log(DISP_1);
    printf("Starting dispensing...\n"
           "Slots left:          %u\n"
           "Dispensing interval: %u s\n");

    uint64_t start = TIME_S;

    for (int slot = 0; slot < slots_left; slot++) {

        uint64_t next_pilling_time = start + PILL_INTERVAL_S * slot;
        while (TIME_S < next_pilling_time) {
            sleep_ms(5);
        }

        set_piezo_flag(false);
        set_piezo_irq(true);
        rotate_8th(1);

        if (!piezo_detection_within_us()) {
            printf("#%d No pill detected.\n", slot + 1);
            led_blink_times(5);
        } else {
            printf("#%d Pill detected.\n", slot + 1);
            ++pills_detected;
        }
        set_piezo_irq(false);
        // log(DISP_2)
    }
    printf("Dispensing finished.\n\n");
    // log(DISP_0);
}