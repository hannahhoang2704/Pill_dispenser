#include <pico/printf.h>
#include "piezo.h"

//initialize piezo sensor pin
void init_piezo() {
    gpio_init(PIEZO_SENSOR);
    gpio_set_dir(PIEZO_SENSOR, GPIO_IN);
    gpio_pull_up(PIEZO_SENSOR);
}

