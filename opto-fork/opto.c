#include <stdio.h>
#include "opto.h"

#define OPTO_GPIO 28

// volatile needed because of interrupt handling
static bool volatile opto_fall = false;
static bool volatile opto_rise = false;

// return the state of opto_fall: whether an edge has been detected
bool opto_flag_state(uint8_t flag) {
    switch (flag) {
        case FALL:
            return opto_fall;
        case RISE:
            return opto_rise;
        default:
            fprintf(stderr, "Unknown flag: %u\n", flag);
            return false;
    }
}

// manual control over flag
void set_opto_flag(uint8_t flag, bool state) {
    switch (flag) {
        case FALL:
            opto_fall = state; break;
        case RISE:
            opto_rise = state; break;
        default:
            fprintf(stderr, "Unknown flag: %u\n", flag);
    }
}

// opto-fork interrupt event
static void opto_event(uint gpio, uint32_t event_mask) {
    switch (event_mask) {
        case GPIO_IRQ_EDGE_FALL:
            opto_fall = true; break;
        case GPIO_IRQ_EDGE_RISE:
            opto_rise = true; break;
        default:
            fprintf(stderr, "Unknown opto-fork interrupt event_mask: %u\n", event_mask);
    }
}

void init_opto_fork_irq() {
    gpio_init(OPTO_GPIO);
    gpio_pull_up(OPTO_GPIO);
    gpio_set_irq_enabled_with_callback(OPTO_GPIO,
                                       GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
                                       true,
                                       opto_event);
}