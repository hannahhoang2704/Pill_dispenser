#include <pico/printf.h>
#include "hardware/i2c.h"
#include "pico/stdlib.h"


#include "LED/LED.h"
#include "switch/switch.h"
#include "stepper/stepper.h"
#include "opto-fork/opto.h"
#include "piezo/piezo.h"
#include "eeprom/eeprom.h"
#include "Lora/Lora.h"

static const uint16_t stored_entry_index_address = I2C_MEMORY_SIZE-1;
static const uint16_t stored_rotation_address = I2C_MEMORY_SIZE-2;

int main() {
    stdio_init_all();
    init_Lora();
    struct LoRaE5 lora;
    connect_network(&lora);

    LED led_3 = {.pin = LED_3};
    init_pwm(&led_3);

    init_switch(SW_0);

    init_opto_fork();
    init_stepper();
    init_piezo();

    initialize_eeprom();

    int rotations = 0; // read from eeprom, zero by default
    uint8_t current_entry_index = 0;

    read_from_eeprom(stored_rotation_address, (uint8_t *)&rotations, 1);
    if (rotations < 0 || rotations > 7) rotations = 0;

    //write first Boot log to memory
    read_from_eeprom(stored_entry_index_address, &current_entry_index, 1);
    if(current_entry_index >= MAX_ENTRIES || current_entry_index < 0) current_entry_index = 0;

    char boot_log[] = "Boot";
    char calibration_started[] = "Calibration started";
    
//    printf("%s\n", boot_log);
    write_log_entry(boot_log, &current_entry_index, stored_entry_index_address);

    while (true) {
        /// AT STARTUP ///
        /// Minimum:
        // Blink LED,
        // Until button pressed
        /// Advanced:
        // "Remembers state across boot/power down":
        // Log to EEPROM
        // LoRaWAN Report: boot.

        while (!switch_pressed_debounced(SW_0)) {
            toggle_pwm(&led_3);
            sleep_ms(500);
            send_message(&lora, boot_log); // Advanced: Lora send boot
        }

        set_led_brightness(&led_3, PWM_OFF);

        /// BUTTON PRESSED #1 ///
        /// Minimum:
        // Calibrate,
        // 'At least one full "turn" (revolution?)',
        // Stops at opto-fork
        /// Advanced:
        // Log to EEPROM
        // Calibrate according to state:
        // opposite direction so the pills aren't dispensed.
        // LoRaWAN Report: Calibration started.

        calibrate(rotations);
        send_message(&lora, calibration_started); // Advanced: Lora send Calibration started

        /// AFTER INITIAL CALIBRATION ///
        /// Minimum:
        // LED turns on
        // [fill pill slots]
        // Waits for button press
        /// Advanced:
        // Log to EEPROM
        // ? Dispense pill if previous dispensing was not finished due to reboot ?
        // LoRaWAN Report: Calibration finished.

        set_led_brightness(&led_3, PWM_SOFT);
        send_message(&lora, "calibration_finished"); // Advanced: Lora send Calibration finished

        while (!switch_pressed_debounced(SW_0)) {
            sleep_ms(50);
        }

        /// BUTTON PRESSED #2 ///
        /// Minimum:
        // LED Off
        // Start dispensing pills at X intervals (X = 30 s).
        /// Advanced:
        // Log to EEPROM
        // LoRaWAN Report: Dispensing started.

        set_led_brightness(&led_3, PWM_OFF);
        send_message(&lora, "dispensing_started"); // Advanced: Lora send Dispensing started

        set_piezo_irq(true);
        uint64_t start = TIME_S;
        for (int pill = rotations; pill < PILL_COUNT; pill++) {

            /// WAIT FOR 30 s INTERVAL: DROP DROP ///
            /// Minimum:
            // Detect pill drop with piezo-sensor (counting steps isn't mentioned... however:),
            // If no pill was detected (within a certain number of steps?), blink LED 5 times
            /// Advanced:
            // Log to EEPROM
            // LoRaWAN Report: Dropping pill...
            // LoRaWAN Report: Pill drop detected || not detected. (Again, determined how?)

            uint64_t next_pilling_time = start + PILL_INTERVAL_S * pill;
            while (TIME_S < next_pilling_time) {
                sleep_ms(5);
            }

            set_piezo_flag(false);
            rotate_8th(1);
            send_message(&lora, "Dropping pill..."); // Advanced: Lora send Dispensing started


            if (!piezo_detection_within_us()) {
                printf("No pill found in compartment %d, blink lights\n", pill + 1);
                led_blink_times(&led_3, BLINK_TIMES);
            } else {
                printf("Pill found in compartment %d\n", pill + 1);
            }
        }
//        printf("Dispenser empty.\n");
        set_piezo_irq(false);

        rotations = 0;

        /// WHEEL TURNED 7 TIMES /// ["All pills dispensed"]
        /// Minimum:
        // ? Confirm that the next "slot" is opto-fork, in order to confirm 'full revolution' ?
        // Loop back to "STARTUP"
        /// Advanced:
        // Log to EEPROM
        // LoRaWAN Report: Pills dispensed.
        // LoRaWAN Report: Restarting operation.
        send_message(&lora, "Pills dispensed"); // Advanced: Lora send Dispensing started
        send_message(&lora, "Restarting operation"); // Advanced: Lora send Dispensing started
    }
}