#include <pico/printf.h>

#include "operation/operation.h"

int main() {
    stdio_init_all();

    printf("\n%s\n", log_msg[BOOT]);

    LED led_3 = init_pwm(LED_3);
    SW sw_0 = init_switch(SW_0);
    init_opto_fork();
    init_stepper();
    init_piezo();
    init_Lora();
    printf("Pins initialized.\n");

    init_eeprom();
    oper_st state = init_operation();
    print_state(state);

    write_log_entry(log_msg[BOOT], &(state.eeprom_log_i));

    while (true) {
        if (state.comp_rotd == MAX_COMP_COUNT) {
            blink_until_sw_pressed(&sw_0, &led_3);
        }
        write_log_entry(log_msg[SW_PRESSED], &(state.eeprom_log_i));

        calibrate(&state);

        wait_until_sw_pressed(&led_3, &sw_0);
        write_log_entry(log_msg[SW_PRESSED], &(state.eeprom_log_i));

        dispense(&state, &led_3);
    }
}