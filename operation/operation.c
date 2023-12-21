#include <pico/stdlib.h>
#include <pico/printf.h>
#include <stdlib.h>
#include <stdarg.h>

#include "operation.h"

// returns system time with decimal accuracy
// and changes its unit depending on its size
uint64_t get_time_with_decimal(char * time_unit) {
    // d as in desi-units as in 10th of 'time'
    uint64_t time_d = time_us_64() / 100000;
    *time_unit = 's';
    if (time_d > 600) {
        if (time_d > 36000) {
            *time_unit = 'h';
            time_d /= 3600;
        } else {
            time_d /= 60;
            *time_unit = 'm';
        }
    }
    return time_d;
}


void logf_msg(enum logs logEnum, oper_st * state, int n_args, ...) {
    char msg[STRLEN_EEPROM];

    char time_unit;
    uint64_t time = get_time_with_decimal(&time_unit);
    snprintf(msg, 9, "[%2llu,%llu %c] ", time / 10, time % 10, time_unit);
    char content[STRLEN_EEPROM - 10];
    va_list args;
    va_start(args, n_args);
    vsnprintf(content, STRLEN_EEPROM - 10, log_msg[logEnum], args);
    va_end(args);
    strcat(msg, content);

    printf("%s\n", msg);
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
        case PILL_0:
            if (state->lora_conn)
                send_msg(msg, true);
            break;
        case ROT_0:
            write_to_eeprom(COMP_ROTD_ADDR,
                            &state->comp_rotd, 1);
            break;
        case CALIB_0_1:
            write_to_eeprom(COMP_ROTD_ADDR,
                            &state->comp_rotd, 1);
            if (state->lora_conn)
                send_msg(msg, true);
            break;
        case PILL_1:
            write_to_eeprom(PILLS_DET_ADDR,
                            &state->pills_det, 1);
            if (state->lora_conn)
                send_msg(msg, true);
    }
}

// generates operations state,
// data based on LoRa connection
// and EEPROM data
oper_st init_operation() {
    init_eeprom();
    oper_st state;

    state.eeprom_log_i = get_reg_value(LOG_INDEX_ADDR);
    logf_msg(BOOT, &state, 0);
    state.comp_rotd = get_reg_value(COMP_ROTD_ADDR);
    if (state.comp_rotd > MAX_COMP_COUNT) state.comp_rotd = MAX_COMP_COUNT;
    state.pills_det = get_reg_value(PILLS_DET_ADDR);
    if (state.pills_det > MAX_COMP_COUNT) state.pills_det = 0;

    init_Lora();
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

// Loops infinitely until switch is pressed,
// blinking LED while looping.
// Leaves LED off after press.
void blink_until_sw_pressed(SW * sw_proceed, LED * led) {
    printf("Press switch #%u to calibrate...\n", sw_proceed->board_index);
    uint8_t loop = 0;
    while (!switch_pressed_debounced(sw_proceed)) {
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

        // rotate clockwise into opto_fork area and count steps,
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
void wait_until_sw_pressed(SW * sw_proceed, LED * led) {
    set_led_brightness(led, PWM_SOFT);
    printf("Press switch #%u to start dispensing...\n",
           sw_proceed->board_index);
    while (!switch_pressed_debounced(sw_proceed)) {
        sleep_ms(50);
    }
    printf("Switch #%u pressed.\n\n",
           sw_proceed->board_index);
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