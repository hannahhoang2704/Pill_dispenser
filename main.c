#include "operation/operation.h"

int main() {
    stdio_init_all();

    oper_st state = init_operation();

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