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

    oper_st state = init_operation();
    initialize_eeprom();

    int rotations = 0; // read from eeprom, zero by default
    uint8_t current_entry_index = 0;

    read_from_eeprom(stored_rotation_address, (uint8_t *)&rotations, 1);
    if (rotations < 0 || rotations > 7) rotations = 0;

    //write first Boot log to memory
    read_from_eeprom(stored_entry_index_address, &current_entry_index, 1);
    if(current_entry_index >= MAX_ENTRIES || current_entry_index < 0) current_entry_index = 0;

    char boot_log[] = "Boot";
//    printf("%s\n", boot_log);
    write_log_entry(boot_log, &current_entry_index, stored_entry_index_address);

    while (true) {
        if (state.comp_rotd == MAX_COMP_COUNT) {
            blink_until_sw_pressed(sw_0, led_3);
        }

        calibrate(&state);

        wait_until_sw_pressed(led_3, sw_0);

        dispense(&state, led_3);
    }
}