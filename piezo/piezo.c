#include <pico/printf.h>
#include "piezo.h"

volatile bool piezo_falling_edge = false;

//initialize piezo sensor pin
void init_piezo() {
    gpio_init(PIEZO_SENSOR);
    gpio_set_dir(PIEZO_SENSOR, GPIO_IN);
    gpio_pull_up(PIEZO_SENSOR);
}

//returns a boolean if object is detected by piezo sensor within a maximum of waiting time
bool piezo_detection_within_us() {
    uint32_t time_start = time_us_64();
    do {
        if (piezo_falling_edge) {
            return true;
        }
    } while ((time_us_64() - time_start) <= PIEZO_MAX_WAITING_TIME);
    return false;
}