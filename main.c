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
#define BLINK_TIME 5

int main() {
    stdio_init_all();

    LED led_3 = {LED_3,
                 INIT_PWM_LEVEL};
    init_pwm(led_3);

    SW sw_0 = {SW_0,
               false};
    init_switch(sw_0);

    //init_opto_fork_irq();
    init_opto_fork();
    init_stepper();

    //init piezo sensor
    init_piezo();

    int full_rev_steps = THEORETICAL_REV;

    while (true) {
        /// AT STARTUP ///
        /// Minimum:
        // Blink LED,
        // Until button pressed
        /// Advanced:
        // "Remembers state across boot/power down":
        // Log to EEPROM
        // LoRaWAN Report: boot.

        while (!is_button_clicked(&sw_0)) {
            toggle_pwm(&led_3);
            sleep_ms(500);
        }

        put_pwm(&led_3, PWM_OFF);

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

        full_rev_steps = calibrate();

        /// AFTER INITIAL CALIBRATION ///
        /// Minimum:
        // LED turns on
        // [fill pill slots]
        // Waits for button press
        /// Advanced:
        // Log to EEPROM
        // ? Dispense pill if previous dispensing was not finished due to reboot ?
        // LoRaWAN Report: Calibration finished.

        put_pwm(&led_3, PWM_SOFT);

        while (!is_button_clicked(&sw_0)) {
            sleep_ms(50);
        }

        /// BUTTON PRESSED #2 ///
        /// Minimum:
        // LED Off
        // Start dispensing pills at X intervals (X = 30 s).
        /// Advanced:
        // Log to EEPROM
        // LoRaWAN Report: Dispensing started.

        put_pwm(&led_3, PWM_OFF);

        uint64_t start = TIME_S;
        for (int pill = 0; pill < PILL_COUNT; pill++) {

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
            rotate_8th(full_rev_steps, 1);
            sleep_ms(10);
            bool pill_detected = piezo_detection_within_us();
            if(!pill_detected){
                //toggle led 5 times
                printf("Can't detect any pill in compartment %d, blink light 5 times\n", pill+1);
                for(int blink=0; blink<(BLINK_TIME*2);++blink){
                    toggle_pwm(&led_3);
                    sleep_ms(200);
                }
            }else{
                printf("Pill in compartment %d is detected by piezo\n", pill+1);
            }
        }

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