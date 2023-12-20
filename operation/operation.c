#include <pico/stdlib.h>
#include <pico/printf.h>
#include <stdlib.h>

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
    start_lora();
    oper_st state = {.lora_conn = connect_network()};
    read_from_eeprom(LOG_INDEX_ADDR,
                     (uint8_t *) &(state.eeprom_log_i),1);
    read_from_eeprom(COMP_ROTD_ADDR,
                     (uint8_t *) &state.comp_rotd,1);
    read_from_eeprom(PILLS_DET_ADDR,
                     (uint8_t *) &state.pills_det,1);
    if (state.comp_rotd > MAX_COMP_COUNT) state.comp_rotd = MAX_COMP_COUNT;
    if (state.pills_det > MAX_COMP_COUNT) state.pills_det = MAX_COMP_COUNT;

    return state;
}

void print_state(struct operation_state state) {
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
void blink_until_sw_pressed(SW * sw, LED * led) {
    printf("Press switch #%u to calibrate...\n", sw->board_index);
    uint8_t loop = 0;
    while (!switch_pressed_debounced(sw)) {
        if (loop++ == 0) toggle_pwm(led);
        loop %= 10;
        sleep_ms(50);
    }
    printf("Switch %u pressed.\n\n", sw->board_index);
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
    printf("Calibrating...\n");
    write_log_entry(log_msg[CALIB_1], &state->eeprom_log_i);
    send_msg(log_msg[CALIB_1]);

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

        printf("Full revolution: %d steps\n"
               "Opto-width: %d steps\n",
               full_rev_steps, opto_width);

        state->comp_rotd = 0;
        write_to_eeprom(COMP_ROTD_ADDR,
                        &(state->comp_rotd), 1);
        write_log_entry(log_msg[CALIB_0_1], &state->eeprom_log_i);

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
        write_log_entry(log_msg[CALIB_0_1], &state->eeprom_log_i);
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

    printf("%s dispensing...\n",
           state->comp_rotd == 0 ?
           "Starting" :
           "Continuing");
    write_log_entry(log_msg[DISP_1], &state->eeprom_log_i);

    set_piezo_irq(true);
    uint64_t start = TIME_S;
    uint8_t comps_left = MAX_COMP_COUNT - state->comp_rotd;
    for (uint8_t comp = 0; comp < comps_left; comp++) {
        uint64_t next_pilling_time = start + PILL_INTERVAL_S * comp;
        while (TIME_S < next_pilling_time) {
            sleep_ms(5);
        }

        write_log_entry(log_msg[ROT_1], &state->eeprom_log_i);
        set_piezo_flag(false);
        rotate_8th(1);
        write_log_entry(log_msg[ROT_0], &state->eeprom_log_i);
        ++state->comp_rotd;
        write_to_eeprom(COMP_ROTD_ADDR,
                        &state->comp_rotd, 1);

        if (!piezo_detection_within_us()) {
            printf("No pill found in compartment %d, blink lights\n", state->comp_rotd);
            // log(PILL_0, state);
            write_log_entry(log_msg[PILL_0], &state->eeprom_log_i);
            led_blink_times(led, BLINK_COUNT);
        } else {
            printf("No pill found in compartment %d, blink lights\n", state->comp_rotd);
            ++state->pills_det;
            write_to_eeprom(PILLS_DET_ADDR,
                            &state->pills_det, 1);
            write_log_entry(log_msg[PILL_0], &state->eeprom_log_i);
            // log(PILL_0, state);
        }
    }

    set_piezo_irq(false);
    printf("Dispensing finished.\n\n");
    state->pills_det = 0;
    write_to_eeprom(PILLS_DET_ADDR,
                    &(state->pills_det), 1);
    write_log_entry(log_msg[DISP_1], &state->eeprom_log_i);
}