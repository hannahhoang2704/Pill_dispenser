#include "hardware/i2c.h"
#include "pico/stdlib.h"

#include "LED/LED.h"
#include "stepper.h"
#include "opto.h"

int main() {
    stdio_init_all();

    LED led = {20};
    init_led(&led);
    init_opto_fork();
    init_stepper();

    while (true) {
        /// AT STARTUP ///
        /// Minimum:
        // Blink LED,
        // Until button pressed
        /// Advanced:
        // Remembers state across boot/power down.
        // LoRaWAN Report: boot.

        /// BUTTON PRESSED #1 ///
        /// Minimum:
        // Calibrate,
        // 'At least one full "turn" (revolution?)',
        // Stops at opto-fork,
        // LED turns on
        /// Advanced:
        // Calibrate according to state:
        // opposite direction so the pills aren't dispensed.
        // LoRaWAN Report: Calibration started.

        /// AFTER INITIAL CALIBRATION ///
        /// Minimum:
        // [fill pill slots]
        // Waits for button press
        /// Advanced:
        // ? Dispense pill if previous dispensing was not finished due to reboot ?
        // LoRaWAN Report: Calibration finished.

        /// BUTTON PRESSED #2 ///
        /// Minimum:
        // Start dispensing pills at X intervals (x = 30 s),
        /// Advanced:
        // LoRaWAN Report: Dispensing started.

        /// PILL DROP ///
        /// Minimum:
        // Detect pill drop with piezo-sensor (counting steps isn't mentioned... however:),
        // If no pill was detected (within a certain number of steps?), blink LED 5 times
        /// Advanced:
        // LoRaWAN Report: Dropping pill...
        // LoRaWAN Report: Pill drop detected || not detected. (Again, determined how?)

        /// WHEEL TURNED 7 TIMES /// ["All pills dispensed"]
        /// Minimum:
        // ? Ensure that the next "slot" is opto-fork, in order to confirm 'full revolution' ?
        // Loop back to "STARTUP"
        /// Advanced:
        // LoRaWAN Report: Pills dispensed.
        // LoRaWAN Report: Restarting operation.
    }
}