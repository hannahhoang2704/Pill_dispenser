#include <pico/stdlib.h>
#include <pico/printf.h>
#include <stdlib.h>
#include <stdarg.h>

#include "operation.h"

// returns system time with decimal accuracy
// and changes its unit depending on its size
uint64_t get_time_with_decimal(char * time_unit) {
    uint64_t time_d = TIME_DS;
    *time_unit = 's';
    if (time_d > SECS_IN_MIN * DEC_OFFSET) {
        if (time_d > SECS_IN_HOUR * DEC_OFFSET) {
            *time_unit = 'h';
            time_d /= SECS_IN_HOUR;
        } else {
            time_d /= SECS_IN_MIN;
            *time_unit = 'm';
        }
    }
    return time_d;
}

// logs a message and possible values
// according to state in question to appropriate places
void logf_msg(enum logs logEnum, oper_st * state, int n_args, ...) {
    char msg[STRLEN_EEPROM];

    char time_unit;
    uint64_t time = get_time_with_decimal(&time_unit);
    snprintf(msg, TIMESTAMP_LEN, "[%2llu,%01llu %c]",
             time / DEC_OFFSET, time % DEC_OFFSET, time_unit);

    char content[STRLEN_EEPROM - TIMESTAMP_LEN];
    va_list args;
    va_start(args, n_args);
    vsnprintf(content, STRLEN_EEPROM - TIMESTAMP_LEN,
              log_format[logEnum], args);
    va_end(args);

    strcat(msg, content);

    printf("%s\n", msg);
    write_log_entry(msg, &state->eeprom_log_i);

    // values to store to eeprom
    switch (logEnum) {
        case CALIBRATED_FULL:
        case COMPARTMENT_ROTATION_FINISHED:
            write_to_eeprom(COMP_ROTD_ADDR,
                            &state->comps_rotd, 1);
            break;
        case PILL_DETECTED:
            write_to_eeprom(PILLS_DET_ADDR,
                            &state->pills_detd, 1);
        default:
            ;
    }

    // LoRa messages to be sent to mqtt
    switch (logEnum) {
        case BOOT:
        case LORA_NOT_CONNECTED:
            break;
        default:
            if (state->lora_conn)
                send_msg(msg, false);
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

    state.comps_rotd = get_reg_value(COMP_ROTD_ADDR);
    state.pills_detd = get_reg_value(PILLS_DET_ADDR);

    if (state.comps_rotd > PILLCOMP_COUNT) state.comps_rotd = PILLCOMP_COUNT;
    if (state.pills_detd > PILLCOMP_COUNT) state.pills_detd = 0;

    init_Lora();
    start_lora();
    if ((state.lora_conn = connect_network())) {
        logf_msg(LORA_CONNECTED, &state, 0);
    } else {
        logf_msg(LORA_NOT_CONNECTED, &state, 0);
    }
    return state;
}

void print_state(oper_st state) {
    printf("lora_conn: %s\n"
           "eeprom_log_i: %hhu\n"
           "comps_rotd: %hhu\n"
           "pills_detd: %hhu\n"
           "\n",
           state.lora_conn ? "true" : "false",
           state.eeprom_log_i,
           state.comps_rotd,
           state.pills_detd);
}

// Loops infinitely until switch is pressed,
// blinking LED while looping.
// Leaves LED off after press.
void blink_until_sw_pressed(SW * sw_proceed, LED * led, oper_st * state) {
    logf_msg(WAITING_FOR_SW, state, 0);
    uint8_t loop = 0;
    while (!switch_pressed_debounced(sw_proceed)) {
        if (loop++ == 0) toggle_pwm(led);
        loop %= 10;
        sleep_ms(50);
    }
    logf_msg(SW_PRESSED, state,
             1, sw_proceed->board_index);
    set_led_brightness(led, PWM_OFF);
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

// Calibrates according to number of 'comps_rotd' done thus far.
// 'comps_rotd' is to be derived from EEPROM
void calibrate(oper_st * state) {

    logf_msg(CALIBRATING_STARTED, state, 0);

    int opto_width;

    if (state->comps_rotd >= PILLCOMP_COUNT) {

        // rotate clockwise into opto-fork area
        rotate_to_event(FALL, true);

        // rotate clockwise out of opto-fork area
        // and measure opto_width
        opto_width = rotate_to_event(RISE, true);

        // rotate clockwise into opto_fork area and count steps,
        // completing full revolution
        int full_rev_minus_opto_width = rotate_to_event(FALL, true);
        int full_revolution = opto_width + full_rev_minus_opto_width;

        // align the disk with the hole,
        // according to measured opto_width minus slippage
        rotate_steps(opto_width / 2 - OPTO_OFFSET);

        // full calibration resets dispensing rotations
        // and empties any possible pills from compartments
        state->comps_rotd = 0;
        state->pills_detd = 0;
        logf_msg(CALIBRATED_FULL, state,
                 1, full_revolution);

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
        rotate_steps(-opto_width / 2 + OPTO_OFFSET);

        // then rotate according to logged
        rotate_8th(state->comps_rotd);
        logf_msg(CALIBRATED_PARTIAL, state,
                 1, state->comps_rotd);
    }
}

// Puts LED on.
// Loops infinitely until switch is pressed.
// Puts LED off after switch press.
void wait_until_sw_pressed(SW * sw_proceed, LED * led, oper_st * state) {
    set_led_brightness(led, PWM_SOFT);
    logf_msg(WAITING_FOR_SW, state, 0);
    while (!switch_pressed_debounced(sw_proceed)) {
        sleep_ms(50);
    }
    logf_msg(SW_PRESSED, state,
             1, sw_proceed->board_index);
    set_led_brightness(led, PWM_OFF);
}

// Dispense pills according 'comps_rotd',
// which is read from EEPROM on boot.
void dispense(oper_st * state, LED * led) {

    logf_msg(DISPENSING_STARTED, state,
             1, state->comps_rotd);

    set_piezo_irq(true);
    uint64_t start = TIME_S;
    uint8_t comps_left = PILLCOMP_COUNT - state->comps_rotd;
    for (uint8_t comp = 0; comp < comps_left; comp++) {
        uint64_t next_pilling_time = start + PILL_INTERVAL_S * comp;
        while (TIME_S < next_pilling_time) {
            sleep_ms(5);
        }

        logf_msg(COMPARTMENT_ROTATION_STARTED, state,
                 1, state->comps_rotd + 1);
        set_piezo_flag(false);
        rotate_8th(1);
        ++state->comps_rotd;
        logf_msg(COMPARTMENT_ROTATION_FINISHED, state, 0);

        if (!piezo_detection_within_us()) {
            logf_msg(PILL_NOT_DETECTED, state,
                     1, state->comps_rotd);
            led_blink_times(led, BLINK_COUNT);
        } else {
            ++state->pills_detd;
            logf_msg(PILL_DETECTED, state,
                     1, state->comps_rotd);
        }
    }

    set_piezo_irq(false);
    logf_msg(DISPENSING_FINISHED, state,
             1, state->pills_detd);
}