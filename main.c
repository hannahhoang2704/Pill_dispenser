#include <stdio.h>
#include "hardware/i2c.h"
#include "pico/stdlib.h"

#include "stepper.h"
#include "opto.h"
#include "command.h"

int main() {
    stdio_init_all();

    init_opto_fork();
    init_stepper();

    /// CALIBRATE ///
    // ... pill cabinet position.
    // 'Count the number of steps per revolution
    // by using an optical sensor.'
    // 'Reducer gear ratio is NOT EXACTLY 1:64'
    // so steps per revolution is NOT 4096 either...???

    ////// USER-INPUT //////
    /// calib /// initiates calibration
    // Run motor in one direction,
    // counting steps,
    // until opto detects falling edge.
    // 'For accuracy, count steps x3 and take average.'
    /// status /// print status according to whether calibrated
    // stepper_is_calibrated()
    // Steps per revolution if calibrated...
    // ... "Unknown" if not calibrated.
    /// run N /// run motor according to N
    // Motor runs N 1/8th's -- N == number of 'cabinet steps'.
    // In case N is omitted, run full revolution.

    char input[MAX_CMD_LEN + EXCESS_C + NULLC];
    int steps_per_rev = -1;
    int octets;
    printf("Waiting for command...\n");
    while (true) {
        st state = valid_command_received(input, &octets);
        if (state != WAIT) {
            switch (state) {
                case CALIBRATE:
                    steps_per_rev = calibrate(); break;
                case STATUS:
                    print_status(steps_per_rev); break;
                case RUN:
                    rotate_octets(steps_per_rev, octets); break;
                default:
                    fprintf(stderr, "Stderr herr derr\n");
            }
            printf("\nWaiting for command...\n");
        }
    }
}