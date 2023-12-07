#include "opto.h"

#define OPTO_GPIO 28

void init_opto_fork() {
    gpio_init(OPTO_GPIO);
    gpio_set_dir(OPTO_GPIO, GPIO_IN);
    gpio_pull_up(OPTO_GPIO);
}

bool opto_high() {
    return gpio_get(OPTO_GPIO);
}