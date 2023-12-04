#include "stepper.h"

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
    static const uint8_t step_masks[STEP_STATES] = {A, AB, B, BC, C, CD, D, DA};

    static const struct coil_struct {
        int gpio;
        uint8_t bit;
    } coils[COIL_COUNT] = {{A_GPIO, A},
                           {B_GPIO, B},
                           {C_GPIO, C},
                           {D_GPIO, D}};

    static uint8_t stepper_mask_i = 0;
    uint8_t step_state = step_masks[stepper_mask_i];

    stepper_mask_i = clockwise ?
                     stepper_mask_i < 7 ?
                     stepper_mask_i + 1 :
                     0 :
                     stepper_mask_i > 1 ?
                     stepper_mask_i - 1 :
                     7;

    for (int coil_i = 0; coil_i < COIL_COUNT; coil_i++) {
        struct coil_struct coil = coils[coil_i];
        gpio_put(coil.gpio, coil.bit & step_state);
    }
}
