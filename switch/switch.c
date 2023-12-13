#include "switch.h"

void init_switch(uint sw) {
    gpio_set_function(sw, GPIO_FUNC_SIO);
    gpio_set_dir(sw, GPIO_IN);
    gpio_pull_up(sw);
}

bool switch_pressed(uint sw) {
    return !gpio_get(sw);
}

static bool sw_0_pressed = false;

bool switch_pressed_debounced(uint sw) {
    if (!gpio_get(sw) && !sw_0_pressed) {
        sw_0_pressed = true;
    } else if (gpio_get(sw) && sw_0_pressed) {
        sw_0_pressed = false;
    }
    return sw_0_pressed;
}