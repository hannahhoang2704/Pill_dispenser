#include "switch.h"

void init_switch(SW sw) {
    gpio_set_function(sw.pin, GPIO_FUNC_SIO);
    gpio_set_dir(sw.pin, GPIO_IN);
    gpio_pull_up(sw.pin);
}

bool switch_pressed(SW sw) {
    return !gpio_get(sw.pin);
}