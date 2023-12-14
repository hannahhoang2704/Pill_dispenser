#include "pico/stdlib.h"

#define THEORETICAL_REV 4096
#define THEORETICAL_8TH 512

void init_stepper();

void step(bool clockwise);

void calibrate(int rotations);

void rotate_8th(int n_8ths);