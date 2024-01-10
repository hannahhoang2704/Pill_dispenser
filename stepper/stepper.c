#include <stdio.h>

#include "stepper.h"

#define IN1 2
#define IN2 3
#define IN3 6
#define IN4 13
#define COIL_COUNT 4

//Initialize stepper motor driver pins IN1-IN4
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

// Rotates steps. Positive steps = clockwise. Negative steps = counter-clockwise
void rotate_steps(int steps) {
    bool clockwise = steps >= 0;
    int start = clockwise ? 0 : steps;
    int target = clockwise ? steps : 0;

    for (int s = start; s != target; s++) {
        step(clockwise);
        sleep_us(STEPPER_WAITING_US);
    }
}

// rotates n * full_revolution / 8
void rotate_8th(int n_8ths) {
    int steps = THEORETICAL_8TH * n_8ths;
    rotate_steps(steps);
}