#include <pico/stdlib.h>
#include <pico/printf.h>
#include <stdlib.h>
#include <stdarg.h>

#include "operation.h"

static volatile bool interrupt_flags[] = {false, false, false};

//irq callback event when detecting event mask from Opto fork pin or piezo sensor pin
static void irq_event(uint gpio, uint32_t event_mask) {
    if(gpio == OPTO_GPIO){
        static uint64_t prev_event_time = 0;
        uint64_t curr_time = time_us_64();

        if (curr_time - prev_event_time > EVENT_DEBOUNCE_US) {
            prev_event_time = curr_time;
            switch (event_mask) {
                case GPIO_IRQ_EDGE_FALL:
                    interrupt_flags[OPTO_FALL] = true;
                    break;
                case GPIO_IRQ_EDGE_RISE:
                    interrupt_flags[OPTO_RISE] = true;
                    break;
                default:;
            }
        }
    }else if(gpio == PIEZO_SENSOR && event_mask == GPIO_IRQ_EDGE_FALL){
        interrupt_flags[PIEZO_FALL] = true;
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

//returns a boolean if object is detected by piezo sensor within a maximum of waiting time
bool piezo_detection_within_us() {
    uint32_t time_start = time_us_64();
    do {
        if (interrupt_flags[PIEZO_FALL]) {
            return true;
        }
    } while ((time_us_64() - time_start) <= PIEZO_MAX_WAITING_TIME);
    return false;
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
        case DISPENSE_CONTINUED:
        case ROTATION_COMPLETED:
            write_to_eeprom(COMP_INDEX_ADDR,
                            &state->current_comp_idx, 1);
            break;
        case PILL_FOUND:
            write_to_eeprom(PILLS_DETECTION_ADDR,
                            &state->pills_detected, 1);
            break;
        default:
            ;
    }

    // Cases where LoRa msg's will be sent, and how
    switch (logEnum) {
        /// Msg sent and response waited for in:
        case LORA_SUCCEED:
        case WAITING_FOR_SW:
        case CALIB_START:
        case CALIB_COMPLETED:
        case RECALIB_AFTER_REBOOT:
        case DISPENSE_CONTINUED:
        case DISPENSE_COMPLETED:
        case PILL_FOUND:
        case NO_PILL_FOUND:
            if (state->lora_connected)
                send_msg_to_Lora(msg, true);
            break;
        /// No msg sent in the rest:
        default: ;
    }
}

// generates operations state, data based on LoRa connection and EEPROM data
oper_st init_operation() {
    init_eeprom();
    oper_st state;

    state.eeprom_log_idx = get_stored_value(LOG_INDEX_ADDR);
    if (state.eeprom_log_idx >= MAX_ENTRIES) state.eeprom_log_idx = 0;
    logf_msg(BOOT, &state, 0);

    state.current_comp_idx = get_stored_value(COMP_INDEX_ADDR);
    state.pills_detected = get_stored_value(PILLS_DETECTION_ADDR);

    if (state.current_comp_idx > PILLCOMP_COUNT) state.current_comp_idx = PILLCOMP_COUNT;
    if (state.pills_detected > PILLCOMP_COUNT) state.pills_detected = 0;

    state.dispensing_underway = state.current_comp_idx != PILLCOMP_COUNT;

    init_Lora();
    start_lora();

    state.lora_connected = connect_network();
    logf_msg(state.lora_connected ? LORA_SUCCEED : LORA_FAILED, &state, 0);

    state.led = init_pwm(LED_3);
    state.sw_proceed = init_switch(SW_0);
    state.sw_log = init_switch(SW_1);

    init_stepper();
    init_opto_fork();
    init_piezo();

    set_opto_fork_irq();
    set_piezo_irq();

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

// Prints stored EEPROM logs.
void press_sw_to_read_log(oper_st * state) {
    if (switch_pressed_debounced(&state->sw_log)) {
        logf_msg(SW_PRESSED, state,
                 1, state->sw_log.board_index);
        read_log_entry(state->current_comp_idx);
    }
}

// Loops infinitely until switch is pressed, blinking LED while looping.
// Leaves LED off after press.
void blink_until_sw_pressed(oper_st * state) {
    logf_msg(WAITING_FOR_SW, state, 2,
             state->sw_proceed.board_index, state->sw_log.board_index);
    uint8_t loop = 0;
    while (!switch_pressed_debounced(&state->sw_proceed)) {
        if (loop++ == 0) toggle_pwm(&state->led);
        loop %= 10;
        press_sw_to_read_log(state);
        sleep_ms(50);
    }
    logf_msg(SW_PRESSED, state,
             1, state->sw_proceed.board_index);
    set_led_brightness(&state->led, PWM_OFF);
}

// to_opto defines whether it will rotate in or out of opto-fork
// returns number of steps taken during this function
int rotate_to_event(enum interrupt_events flag, bool clockwise) {
    int steps = 0;
    interrupt_flags[flag] = false;
    while (!interrupt_flags[flag]) {
        step(clockwise);
        ++steps;
        sleep_us(STEPPER_WAITING_US);
    }
    return steps;
}

// Calibrates according to number of 'current_comp_idx' done thus far.
// 'current_comp_idx' is to be derived from EEPROM
void calibrate(oper_st * state) {

    logf_msg(CALIB_START, state, 0);

    int opto_width;

    if (!state->dispensing_underway) {

        // rotate clockwise into opto-fork area
        rotate_to_event(OPTO_FALL, true);

        // rotate clockwise out of opto-fork area
        // and measure opto_width
        opto_width = rotate_to_event(OPTO_RISE, true);

        // rotate clockwise into opto_fork area and count steps,
        // completing full revolution
        int full_rev_minus_opto_width = rotate_to_event(OPTO_FALL, true);
        int full_rev_steps = opto_width + full_rev_minus_opto_width;

        // align the disk with the hole,
        // according to measured opto_width minus slippage
        rotate_steps(opto_width / 2 - OPTO_OFFSET);

        state->current_comp_idx = 0;
        state->pills_detected = 0;
        logf_msg(CALIB_COMPLETED, state, 1, full_rev_steps);

    } else {

        // rotate counter-clockwise beyond opto-fork area
        rotate_to_event(OPTO_RISE, false);

        sleep_ms(50);

        // rotate counter-clockwise out of opto-fork
        // and measure opto_width
        opto_width = rotate_to_event(OPTO_RISE, true);

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
void wait_until_sw_pressed(oper_st * state) {
    set_led_brightness(&state->led, PWM_SOFT);
    logf_msg(WAITING_FOR_SW, state, 2,
             state->sw_proceed.board_index, state->sw_log.board_index);
    while (!switch_pressed_debounced(&state->sw_proceed)) {
        press_sw_to_read_log(state);
        sleep_ms(50);
    }
    logf_msg(SW_PRESSED, state,
             1, state->sw_proceed.board_index);
    set_led_brightness(&state->led, PWM_OFF);
}

// Dispense pills according 'current_comp_idx', which is read from EEPROM on boot.
void dispense(oper_st * state) {
    state->dispensing_underway = true;
    logf_msg(DISPENSE_CONTINUED, state, 1, state->current_comp_idx);

    uint64_t start = TIME_S;
    uint8_t compartment_left = PILLCOMP_COUNT - state->current_comp_idx;
    for (uint8_t comp = 0; comp < compartment_left; comp++) {
        uint64_t next_pilling_time = start + PILL_INTERVAL_S * comp;
        while (TIME_S < next_pilling_time) {
            press_sw_to_read_log(state);
            sleep_ms(5);
        }

        logf_msg(ROTATION_CONTINUED, state, 1, state->current_comp_idx + 1);
        interrupt_flags[PIEZO_FALL] = false;
        rotate_8th(1);
        ++(state->current_comp_idx);
        logf_msg(ROTATION_COMPLETED, state, 0);

        if (!piezo_detection_within_us()) {
            logf_msg(NO_PILL_FOUND, state, 1, state->current_comp_idx);
            led_blink_times(&state->led, BLINK_COUNT);
        } else {
            ++(state->pills_detected);
            logf_msg(PILL_FOUND, state, 1, state->current_comp_idx);
        }
    }
    state->dispensing_underway = false;

    logf_msg(DISPENSE_COMPLETED, state, 1, state->pills_detected);
}