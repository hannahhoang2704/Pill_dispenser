#include <stdlib.h>
#include <stdio.h>

#include "stepper.h"
#include "opto.h"

#define A_GPIO 2
#define B_GPIO 3
#define C_GPIO 6
#define D_GPIO 13
#define COIL_COUNT 4

void init_stepper() {
    static const int stepper_pin[COIL_COUNT] = {A_GPIO,
                                                B_GPIO,
                                                C_GPIO,
                                                D_GPIO};

    for (int stepper_i = 0; stepper_i < COIL_COUNT; stepper_i++) {
        gpio_init(stepper_pin[stepper_i]);
        gpio_set_dir(stepper_pin[stepper_i], GPIO_OUT);
    }
}

enum step_stage {
    A  = 0b0001,
    AB = 0b0011,
    B  = 0b0010,
    BC = 0b0110,
    C  = 0b0100,
    CD = 0b1100,
    D  = 0b1000,
    DA = 0b1001
};

#define STEP_STATES 8

void step(bool clockwise) {
    static const uint8_t step_masks[STEP_STATES] =
            {A, AB, B, BC, C, CD, D, DA};

    static const struct coil_struct {
        int gpio;
        uint8_t bit;
    } coils[COIL_COUNT] = {{A_GPIO, A},
                           {B_GPIO, B},
                           {C_GPIO, C},
                           {D_GPIO, D}};

    static int8_t stepper_mask_i = 0;

    if (clockwise) {
        if (++stepper_mask_i > 7) stepper_mask_i = 0;
    } else {
        if (--stepper_mask_i < 0) stepper_mask_i = 7;
    }

    uint8_t step_state = step_masks[stepper_mask_i];

    for (int coil_i = 0; coil_i < COIL_COUNT; coil_i++) {
        struct coil_struct coil = coils[coil_i];
        gpio_put(coil.gpio, coil.bit & step_state);
    }
}

// Max stepper speed the disk can keep up with (according to Roni's experience)
#define SPD_REDUC_MIN 800
// Initial speed, for helping the disk accumulate speed gradually,
// helping it build some momentum
#define SPD_REDUC_INIT 1200

#define CALIB_OPERATIONS 4
#define OPTO_OFFSET 158

int calibrate() {
    uint64_t step_shift = SPD_REDUC_INIT;

    printf("Calibrating...\n");

    const bool operation_order[CALIB_OPERATIONS] = {false,
                                                    true,
                                                    false,
                                                    true};
    int steps = 0;

    for (int oper_i = 0; oper_i < CALIB_OPERATIONS; oper_i++) {
        bool condition = operation_order[oper_i];
        while (opto_high() == condition) {
            step(true);
            if (oper_i > 1) ++steps;
            sleep_us(step_shift);
            step_shift = step_shift > SPD_REDUC_MIN ?
                         step_shift - 1 :
                         SPD_REDUC_MIN;
        }
    }

    for (int s = 0; s < OPTO_OFFSET; s++) {
        step(true);
        sleep_us(step_shift);
        step_shift = step_shift < SPD_REDUC_INIT ?
                     step_shift + 20 :
                     SPD_REDUC_INIT;
    }

    printf("%d steps for full revolution.\n", steps);

    return steps;
}

#define THEORETICAL_REV 4096

void rotate_8th(int full_rev, int n_8ths) {
    full_rev = full_rev == -1 ? THEORETICAL_REV : full_rev;
    int steps_per_octet = full_rev / 8;
    bool direction = n_8ths >= 0;
    int target = n_8ths != 0 ?
                 steps_per_octet * abs(n_8ths) :
                 full_rev;

    uint64_t step_shift = SPD_REDUC_INIT;
    for (int step_i = 0; step_i < target; step_i++) {
        step(direction);
        sleep_us(step_shift);
        step_shift = step_shift > SPD_REDUC_MIN ?
                     step_shift - 1 :
                     SPD_REDUC_MIN;
    }
}