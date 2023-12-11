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
/*
// opto-fork interrupt event
static void opto_event(uint gpio, uint32_t event_mask) {
    opto_falling_edge = true;
}

void unflag_opto_event() {
    opto_falling_edge = false;
}

void init_opto_fork_irq() {
    gpio_set_irq_enabled_with_callback(OPTO_GPIO,
                                       GPIO_IRQ_EDGE_FALL,
                                       true,
                                       opto_event);
}
*/