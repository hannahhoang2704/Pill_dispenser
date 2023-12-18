#include "pico/stdlib.h"

#define IN1 2
#define IN2 3
#define IN3 6
#define IN4 13
#define COIL_COUNT 4

#define THEORETICAL_REV 4096
#define THEORETICAL_8TH 512
#define STEP_STATES 8
#define SPD_REDUC_MIN 850
#define OPTO_OFFSET 148

static const struct coil_struct {
    int gpio;
    uint8_t bit;
} coils[COIL_COUNT] = {{IN1, 0b0001},
                       {IN2, 0b0010},
                       {IN3, 0b0100},
                       {IN4, 0b1000}};

static const uint8_t step_masks[STEP_STATES] = {0b0001,
                                                0b0011,
                                                0b0010,
                                                0b0110,
                                                0b0100,
                                                0b1100,
                                                0b1000,
                                                0b1001};

void init_stepper();

void step(bool clockwise);

void rotate_steps(int steps);

void rotate_8th(int n_8ths);