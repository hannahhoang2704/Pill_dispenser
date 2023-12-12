#include <stdio.h>
#include "opto.h"

#define OPTO_GPIO 28

static bool volatile opto_fall = false;
static bool volatile opto_rise = false;

// Returns the state of 'flag'.
bool opto_flag_state(enum opto_flags flag) {
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

// Manual control over flag state.
void set_opto_flag(enum opto_flags flag, bool state) {
    switch (flag) {
        case FALL:
            opto_fall = state; break;
        case RISE:
            opto_rise = state; break;
        default:
            fprintf(stderr, "Unknown flag: %u\n", flag);
    }
}

// Opto-fork interrupt event.
// Enables detecting movement both "into" opto-fork,
// aswell as "out" from opto-fork.
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

// Initialize opto-fork for both falling and rising edge events.
void init_opto_fork_irq() {
    gpio_init(OPTO_GPIO);
    gpio_pull_up(OPTO_GPIO);
    gpio_set_irq_enabled_with_callback(OPTO_GPIO,
                                       GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
                                       true,
                                       opto_event);
}