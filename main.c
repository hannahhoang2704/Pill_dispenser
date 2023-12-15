#include <pico/printf.h>
#include "hardware/i2c.h"
#include "pico/stdlib.h"

#include "LED/LED.h"
#include "switch/switch.h"
#include "stepper/stepper.h"
#include "opto-fork/opto.h"
#include "piezo/piezo.h"

#define TIME_S ((unsigned long) time_us_64() / 1000000)
#define PILL_INTERVAL_S 10
#define PILL_COUNT 7
#define BLINK_TIMES 5

int main() {
    stdio_init_all();

    LED led_3 = {.pin = LED_3};
    init_pwm(&led_3);

    init_switch(SW_0);

    init_opto_fork();
    init_stepper();
    init_piezo();

    int rotations = 0; // read from eeprom, zero by default

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
    }
}