#include <stdlib.h>
#include <stdio.h>

#include "stepper.h"
#include "../opto-fork/opto.h"

void init_stepper() {
    for (int stepper_i = 0; stepper_i < COIL_COUNT; stepper_i++) {
        gpio_init(coils[stepper_i].gpio);
        gpio_set_dir(coils[stepper_i].gpio, GPIO_OUT);
    }
}

// Takes one 'half'-step in the ordered direction.
// Saves the step state within boot ;; not across boots.
void step(bool clockwise) {
    static int8_t stepper_mask_i = 0;

    if (clockwise) {
        if (++stepper_mask_i > 7) stepper_mask_i = 0;
    } else {
        if (--stepper_mask_i < 0) stepper_mask_i = 7;
    }

    uint8_t step = step_masks[stepper_mask_i];

    for (int coil_i = 0; coil_i < COIL_COUNT; coil_i++) {
        struct coil_struct coil = coils[coil_i];
        gpio_put(coil.gpio, coil.bit & step);
    }
}

// Rotates steps.
// Positive steps = clockwise
// Negative steps = counter-clockwise
void rotate_steps(int steps) {
    bool clockwise = steps >= 0;
    int start = clockwise ? 0 : steps;
    int target = clockwise ? steps : 0;

    for (int s = start; s != target; s++) {
        step(clockwise);
        sleep_us(SPD_REDUC_MIN);
    }
}

// rotates n * full_revolution / 8
void rotate_8th(int n_8ths) {
    int steps = THEORETICAL_8TH * n_8ths;

    rotate_steps(steps);
}

// to_opto defines whether it will rotate in or out of opto-fork
int rotate_to_event(enum opto_events flag, bool clockwise) {
    int steps = 0;

    set_opto_fork_irq(true);

    while (!opto_flag_state(flag)) {
        step(clockwise);
        ++steps;
        sleep_us(SPD_REDUC_MIN);
    }

    set_opto_fork_irq(false);

    set_opto_flag(flag, false);
    return steps;
}

// Calibrates according to number of 'rotations' done thus far.
// 'rotations' is to be derived from EEPROM
void calibrate(int rotations) {

    printf("Calibrating...\n");

    if (rotations == 0) {

        // rotate clockwise until opto-fork falling edge
        rotate_to_event(FALL, true);

        // repeat, and capture the number of steps
        int revolution_steps = rotate_to_event(FALL, true);

        // align the disk with the hole
        // offset may vary among devices
        rotate_steps(OPTO_OFFSET);

        printf("%d steps for full revolution.\n", revolution_steps);

    } else {

        // rotate counter-clockwise until opto-fork falling edge
        rotate_to_event(FALL, false);

        // align the disk with the hole
        // offset may vary among devices
        rotate_steps(-OPTO_OFFSET);

        // sleep to mitigate momentum,
        // before rotating opposite direction
        sleep_ms(50);

        // then rotate according to logged
        rotate_8th(rotations);
    }
    printf("Calibration finished\n");
}