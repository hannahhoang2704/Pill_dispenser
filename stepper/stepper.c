#include <stdlib.h>
#include <stdio.h>

#include "stepper.h"
#include "../opto-fork/opto.h"

#define IN1 2
#define IN2 3
#define IN3 6
#define IN4 13
#define COIL_COUNT 4

void init_stepper() {
    static const int stepper_pin[COIL_COUNT] = {IN1,
                                                IN2,
                                                IN3,
                                                IN4};

    for (int stepper_i = 0; stepper_i < COIL_COUNT; stepper_i++) {
        gpio_init(stepper_pin[stepper_i]);
        gpio_set_dir(stepper_pin[stepper_i], GPIO_OUT);
    }
}

#define STEP_STATES 8

void step(bool clockwise) {
    static const uint8_t step_masks[STEP_STATES] =
            {0b0001,
             0b0011,
             0b0010,
             0b0110,
             0b0100,
             0b1100,
             0b1000,
             0b1001};

    static const struct coil_struct {
        int gpio;
        uint8_t bit;
    } coils[COIL_COUNT] = {{IN1, 0b0001},
                           {IN2, 0b0010},
                           {IN3, 0b0100},
                           {IN4, 0b1000}};

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

#define SPD_REDUC_MIN 850

#define OPTO_OFFSET 158

void rotate_steps(int steps) {
    bool clockwise = steps >= 0;
    int start = clockwise ? 0 : steps;
    int target = clockwise ? steps : 0;

    for (int s = start; s != target; s++) {
        step(clockwise);
        sleep_us(SPD_REDUC_MIN);
    }
}

void rotate_8th(int n_8ths) {
    int steps = THEORETICAL_8TH * n_8ths;

    rotate_steps(steps);
}

// to_opto defines whether it will rotate in or out of opto-fork
int rotate_to_event(uint8_t flag, bool clockwise) {
    int steps = 0;
    while (!opto_flag_state(flag)) {
        step(clockwise);
        ++steps;
        sleep_us(SPD_REDUC_MIN);
    }
    set_opto_flag(flag, false);
    return steps;
}

// calibrate according to number of pills dropped so far
// ... informed from EEPROM after boot ;
// 0 otherwise
void calibrate(int pills_dropped) {

    printf("Calibrating...\n");

    if (pills_dropped == 0) {
        rotate_to_event(FALL, true);
        int revolution_steps = rotate_to_event(FALL, true);

        rotate_steps(OPTO_OFFSET);

        printf("%d steps for full revolution.\n", revolution_steps);

    } else {
        rotate_to_event(FALL, false);
        rotate_steps(-OPTO_OFFSET);
        sleep_ms(50);
        rotate_8th(pills_dropped);
    }
}