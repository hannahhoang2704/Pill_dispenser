#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define SW_0 9
#define SW_1 8
#define SW_2 7

void init_switch(uint sw);

bool switch_pressed(uint sw);

bool switch_pressed_debounced(uint sw);