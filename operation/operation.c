#include <pico/stdlib.h>
#include <pico/printf.h>
#include <stdlib.h>
#include <stdarg.h>

#include "operation.h"

extern volatile bool opto_fall;
extern volatile bool opto_rise;
extern volatile bool piezo_falling_edge;

//irq callback event when detecting event mask from Opto fork pin or piezo sensor pin
static void irq_event(uint gpio, uint32_t event_mask) {
    if(gpio == OPTO_GPIO){
        static uint64_t prev_event_time = 0;
        uint64_t curr_time = time_us_64();

        if (curr_time - prev_event_time > EVENT_DEBOUNCE_US) {
            prev_event_time = curr_time;
            switch (event_mask) {
                case GPIO_IRQ_EDGE_FALL:
                    opto_fall = true;
                    break;
                case GPIO_IRQ_EDGE_RISE:
                    opto_rise = true;
                    break;
                default:;
            }
        }
    }else if(gpio == PIEZO_SENSOR && event_mask == GPIO_IRQ_EDGE_FALL){
        piezo_falling_edge = true;
    }

}

// set opto_fork interrupt detection with callback event
void set_opto_fork_irq() {
    gpio_set_irq_enabled_with_callback(OPTO_GPIO,
                                       GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
                                       true,
                                       &irq_event);
}

//set piezo interrupt detection with callback event
void set_piezo_irq() {
    gpio_set_irq_enabled_with_callback(PIEZO_SENSOR,
                                       GPIO_IRQ_EDGE_FALL,
                                       true,
                                       &irq_event);
}

// returns system time with decimal accuracy and changes its unit depending on its size
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

// Log messages of different state to EEPROM, print to stdout and send to LORA
void logf_msg(enum logs logEnum, oper_st * state, int n_args, ...) {
    char msg[STRLEN_EEPROM];
    char time_unit;
    uint64_t time = get_time_with_decimal(&time_unit);
    snprintf(msg, TIMESTAMP_LEN, "[%2llu,%01llu %c]",
             time / DEC_OFFSET, time % DEC_OFFSET, time_unit);
    char content[STRLEN_EEPROM - TIMESTAMP_LEN];
    va_list args;
    va_start(args, n_args);
    vsnprintf(content, STRLEN_EEPROM - 1 - TIMESTAMP_LEN,
              log_format[logEnum], args);
    va_end(args);
    strncat(msg, content, STRLEN_EEPROM - 1 - TIMESTAMP_LEN);

    printf("%s\n", msg);
    write_log_entry(msg, &state->eeprom_log_idx);

    // values to store to eeprom
    switch (logEnum) {
        case CALIB_COMPLETED:
        case ROTATION_COMPLETED:
            write_to_eeprom(COMP_INDEX_ADDR,
                            &state->current_comp_idx, 1);
            break;
        case PILL_FOUND:
            write_to_eeprom(PILLS_DETECTION_ADDR,
                            &state->pills_detected, 1);
        default:
            ;
    }

    // LoRa messages to be sent to mqtt
    switch (logEnum) {
        case BOOT:
        case LORA_FAILED:
            break;
        default:
            if (state->lora_connected)
                send_msg_to_Lora(msg, false);
    }
}

// generates operations state, data based on LoRa connection and EEPROM data
oper_st init_operation() {
    init_eeprom();
    oper_st state;

    state.eeprom_log_idx = get_stored_value(LOG_INDEX_ADDR);
    logf_msg(BOOT, &state, 0);

    state.current_comp_idx = get_stored_value(COMP_INDEX_ADDR);
    state.pills_detected = get_stored_value(PILLS_DETECTION_ADDR);

    if (state.current_comp_idx > PILLCOMP_COUNT) state.current_comp_idx = PILLCOMP_COUNT;
    if (state.pills_detected > PILLCOMP_COUNT) state.pills_detected = 0;

    init_Lora();
    start_lora();
    if ((state.lora_connected = connect_network())) {
        logf_msg(LORA_SUCCEED, &state, 0);
    } else {
        logf_msg(LORA_FAILED, &state, 0);
    }
    return state;
}

void print_state(oper_st state) {
    printf("lora_connected: %s\n"
           "eeprom_log_index: %hhu\n"
           "current_comp_idx: %hhu\n"
           "pills_detected: %hhu\n"
           "\n",
           state.lora_connected ? "true" : "false",
           state.eeprom_log_idx,
           state.current_comp_idx,
           state.pills_detected);
}

// Loops infinitely until switch is pressed, blinking LED while looping.
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
        sleep_us(STEPPER_WAITING_US);
    }
    set_opto_fork_irq(false);
    return steps;
}

// Calibrates according to number of 'current_comp_idx' done thus far.
// 'current_comp_idx' is to be derived from EEPROM
void calibrate(oper_st * state) {

    logf_msg(CALIB_START, state, 0);

    int opto_width;

    if (state->current_comp_idx >= PILLCOMP_COUNT) {

        // rotate clockwise into opto-fork area
        rotate_to_event(FALL, true);

        // rotate clockwise out of opto-fork area
        // and measure opto_width
        opto_width = rotate_to_event(RISE, true);

        // rotate clockwise into opto_fork area and count steps,
        // completing full revolution
        int full_rev_minus_opto_width = rotate_to_event(FALL, true);
        int full_rev_steps = opto_width + full_rev_minus_opto_width;

        // align the disk with the hole,
        // according to measured opto_width minus slippage
        rotate_steps(opto_width / 2 - OPTO_OFFSET);

        state->current_comp_idx = 0;
        state->pills_detected = 0;
        logf_msg(CALIB_COMPLETED, state, 1, full_rev_steps);

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
        rotate_8th(state->current_comp_idx);
        logf_msg(RECALIB_AFTER_REBOOT, state, 1, state->current_comp_idx);
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

// Dispense pills according 'current_comp_idx', which is read from EEPROM on boot.
void dispense(oper_st * state, LED * led) {

    logf_msg(DISPENSE_CONTINUED, state, 1, state->current_comp_idx);

    uint64_t start = TIME_S;
    uint8_t compartment_left = PILLCOMP_COUNT - state->current_comp_idx;
    for (uint8_t comp = 0; comp < compartment_left; comp++) {
        uint64_t next_pilling_time = start + PILL_INTERVAL_S * comp;
        while (TIME_S < next_pilling_time) {
            sleep_ms(5);
        }
        logf_msg(ROTATION_CONTINUED, state, 1, state->current_comp_idx + 1);
        piezo_falling_edge = false;
        rotate_8th(1);
        ++(state->current_comp_idx);
        logf_msg(ROTATION_COMPLETED, state, 0);

        if (!piezo_detection_within_us()) {
            logf_msg(NO_PILL_FOUND, state, 1, state->current_comp_idx);
            led_blink_times(led, BLINK_COUNT);
        } else {
            ++(state->pills_detected);
            logf_msg(PILL_FOUND, state, 1, state->current_comp_idx);
        }
    }

    set_piezo_irq(false);
    logf_msg(DISPENSE_COMPLETED, state,
             1, state->pills_detected);
}