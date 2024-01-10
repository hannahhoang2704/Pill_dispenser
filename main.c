#include <pico/printf.h>

#include "operation/operation.h"

int main() {
    stdio_init_all();

    oper_st state = init_operation();
    init_opto_fork();
    init_stepper();
    init_piezo();

    set_opto_fork_irq();
    set_piezo_irq();

    while (true) {
        if (state.current_comp_idx == PILLCOMP_COUNT) {
            blink_until_sw_pressed(&state);
        }

        calibrate(&state);
        if (state.current_comp_idx == PILLCOMP_COUNT) {
            wait_until_sw_pressed(&state);
        }

        dispense(&state);
    }
}