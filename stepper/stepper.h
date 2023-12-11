#include "pico/stdlib.h"

#define THEORETICAL_REV 4096

void init_stepper();

void step(bool clockwise);

int calibrate();

int new_calibrate();

void rotate_8th(int full_rev, int n_8ths);