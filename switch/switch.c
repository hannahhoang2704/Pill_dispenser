#include "switch.h"

void init_switch(SW sw) {
    gpio_set_function(sw.pin, GPIO_FUNC_SIO);
    gpio_set_dir(sw.pin, GPIO_IN);
    gpio_pull_up(sw.pin);
}

bool switch_pressed(SW sw) {
    return !gpio_get(sw.pin);
}

bool is_button_clicked(SW * sw) {
    if (!gpio_get(sw->pin) && sw->pressed == false) {
        sw->pressed = true;
        return true;
    } else if (gpio_get(sw->pin) && sw->pressed == true) {
        sw->pressed = false;
    }
    return false;
}