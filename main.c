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
        if (!state.dispensing_underway) {
            blink_until_sw_pressed(&state);
        }

        calibrate(&state);

        if (!state.dispensing_underway) {
            wait_until_sw_pressed(&state);
        }

        dispense(&state);
    }
}