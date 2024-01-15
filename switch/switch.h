#ifndef SWITCH_H
#define SWITCH_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdlib.h>

#define SW_0 9
#define SW_1 8
#define SW_2 7

typedef struct SW_S {
    uint pin; // gpio
    bool pressed; // used for debouncing
    uint8_t board_index; // the index named on the board
} SW;

SW init_switch(uint sw_pin);
bool switch_pressed_debounced(SW * sw);

#endif //SWITCH_H