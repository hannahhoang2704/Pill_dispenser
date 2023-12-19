#include "switch.h"

SW init_switch(uint sw_pin) {
    SW sw = {.pin = sw_pin,
             .pressed = false,
             .board_index = abs(((int)sw.pin - 7) - 2)};

    gpio_set_function(sw.pin, GPIO_FUNC_SIO);
    gpio_set_dir(sw.pin, GPIO_IN);
    gpio_pull_up(sw.pin);

    return sw;
}

bool switch_pressed(SW sw) {
    return !gpio_get(sw.pin);
}

bool switch_pressed_debounced(SW sw) {
    if (!gpio_get(sw.pin) && !sw.pressed) {
        sw.pressed = true;
    } else if (gpio_get(sw.pin) && sw.pressed) {
        sw.pressed = false;
    }
    return sw.pressed;
}