#include <pico/printf.h>

#include "operation/operation.h"

int main() {
    stdio_init_all();

    oper_st state = init_operation();
    LED led_3 = init_pwm(LED_3);
    SW sw_proceed = init_switch(SW_0);
    init_opto_fork();
    init_stepper();
    init_piezo();

    //print_state(state);

    while (true) {
        if (state.comps_rotd == PILLCOMP_COUNT) {
            blink_until_sw_pressed(&sw_proceed, &led_3, &state);
        }

        calibrate(&state);

        wait_until_sw_pressed(&sw_proceed, &led_3, &state);

        dispense(&state, &led_3);

        read_log_entry(MAX_ENTRIES);
    }
}