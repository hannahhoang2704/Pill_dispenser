#include <pico/stdlib.h>
#include <pico/printf.h>
#include <stdlib.h>
#include <stdarg.h>

#include "operation.h"

// returns system time
// and changes time_unit according to its size
uint64_t convert_time(char * time_unit) {
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
    return time_s;
}

// generates operations state,
// data based on LoRa connection
// and EEPROM data
oper_st init_operation() {
    init_eeprom();
    oper_st state;
    read_from_eeprom(LOG_INDEX_ADDR,
                     (uint8_t *) &state.eeprom_log_i,1);
    read_from_eeprom(COMP_ROTD_ADDR,
                     (uint8_t *) &state.comp_rotd,1);
    read_from_eeprom(PILLS_DET_ADDR,
                     (uint8_t *) &state.pills_det,1);
    logf_msg(BOOT, &state, 0);

    if (state.comp_rotd > MAX_COMP_COUNT) state.comp_rotd = MAX_COMP_COUNT;
    if (state.pills_det > MAX_COMP_COUNT) state.pills_det = MAX_COMP_COUNT;
    start_lora();
    if ((state.lora_conn = connect_network())) {
        logf_msg(LORA_1, &state, 0);
    } else {
        logf_msg(LORA_0, &state, 0);
    }
    return state;
}

void print_state(oper_st state) {
    printf("lora_conn: %s\n"
           "eeprom_log_i: %hhu\n"
           "comp_rotd: %hhu\n"
           "pills_det: %hhu\n"
           "\n",
           state.lora_conn ? "true" : "false",
           state.eeprom_log_i,
           state.comp_rotd,
           state.pills_det);
}

void logf_msg(enum logs logEnum, oper_st * state, int n_args, ...) {
    char msg[STRLEN_EEPROM];
    va_list args;
    va_start(args, n_args);
    vsnprintf(msg, STRLEN_EEPROM - 1, log_msg[logEnum], args);
    va_end(args);

    printf("%s\n\n", msg);
    write_log_entry(msg, &state->eeprom_log_i);
    write_to_eeprom(LOG_INDEX_ADDR, &state->eeprom_log_i, 1);

    // msg handling is very slow
    switch (logEnum) {
        case LORA_1:
        case LORA_0:
        case CALIB_1:
        case CALIB_0_2:
        case DISP_1:
        case DISP_0:
            send_msg(msg, true);
            break;
        case CALIB_0_1:
        case ROT_0:
            write_to_eeprom(COMP_ROTD_ADDR,
                            &state->comp_rotd, 1);
            break;
        case PILL_1:
            write_to_eeprom(PILLS_DET_ADDR,
                            &state->pills_det, 1);
        case PILL_0:
            send_msg(msg, true);
            break;
    }
}

// Loops infinitely until switch is pressed,
// blinking LED while looping.
// Leaves LED off after press.
void blink_until_sw_pressed(SW * sw, LED * led) {
    printf("Press switch #%u to calibrate...\n", sw->board_index);
    uint8_t loop = 0;
    while (!switch_pressed_debounced(sw)) {
        if (loop++ == 0) toggle_pwm(led);
        loop %= 10;
        sleep_ms(50);
    }
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

// Calibrates according to number of 'comp_rotd' done thus far.
// 'comp_rotd' is to be derived from EEPROM
void calibrate(oper_st * state) {

    logf_msg(CALIB_1, state, 0);

    int opto_width;
    int full_rev_minus_opto_width;
    int full_rev_steps;

    if (state->comp_rotd == 7) {

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

        state->comp_rotd = 0;
        logf_msg(CALIB_0_1, state, 1, full_rev_steps);

    } else {

        // rotate counter-clockwise beyond opto-fork area
        rotate_to_event(RISE, false);

        sleep_ms(50);

        // rotate counter-clockwise out of opto-fork
        // and measure opto_width
        opto_width = rotate_to_event(RISE, true);

        sleep_ms(50);

        // rotate counter-clockwise;
        // align the disk with the hole
        // which is half of opto_width + slippage
        rotate_steps(-opto_width / 2 + 10);

        // then rotate according to logged
        rotate_8th(state->comp_rotd);
        logf_msg(CALIB_0_2, state, 1, state->comp_rotd);
    }
    printf("Calibration finished!\n\n");
}

// Puts LED on.
// Loops infinitely until switch is pressed.
// Puts LED off after switch press.
void wait_until_sw_pressed(LED * led, SW * sw) {
    set_led_brightness(led, PWM_SOFT);
    printf("Press switch #%u to start dispensing...\n",
           sw->board_index);
    while (!switch_pressed_debounced(sw)) {
        sleep_ms(50);
    }
    printf("Switch #%u pressed.\n\n",
           sw->board_index);
    set_led_brightness(led, PWM_OFF);
}

// Dispense pills according 'comp_rotd',
// which is read from EEPROM on boot.
void dispense(oper_st * state, LED * led) {

    logf_msg(DISP_1, state, 1, state->comp_rotd);

    set_piezo_irq(true);
    uint64_t start = TIME_S;
    uint8_t comps_left = MAX_COMP_COUNT - state->comp_rotd;
    for (uint8_t comp = 0; comp < comps_left; comp++) {
        uint64_t next_pilling_time = start + PILL_INTERVAL_S * comp;
        while (TIME_S < next_pilling_time) {
            sleep_ms(5);
        }

        logf_msg(ROT_1, state, 1, state->comp_rotd + 1);
        set_piezo_flag(false);
        rotate_8th(1);
        ++state->comp_rotd;
        logf_msg(ROT_0, state, 0);

        if (!piezo_detection_within_us()) {
            logf_msg(PILL_0, state, 1, state->comp_rotd);
            led_blink_times(led, BLINK_COUNT);
        } else {
            ++state->pills_det;
            logf_msg(PILL_1, state, 1, state->comp_rotd);
        }
    }

    set_piezo_irq(false);
    logf_msg(DISP_0, state, 1, state->pills_det);
    state->pills_det = 0;
    write_to_eeprom(PILLS_DET_ADDR,
                    &state->pills_det, 1);
}