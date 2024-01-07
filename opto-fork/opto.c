#include <stdio.h>
#include "opto.h"

volatile bool opto_fall = false;
volatile bool opto_rise = false;

// Initialize opto-fork.
void init_opto_fork() {
    gpio_init(OPTO_GPIO);
    gpio_pull_up(OPTO_GPIO);
}

// Returns the state of opto detection 'event'.
bool opto_flag_state(enum opto_events event) {
    switch (event) {
        case FALL:
            return opto_fall;
        case RISE:
            return opto_rise;
        default:
            fprintf(stderr, "Unknown event: %u\n", event);
            return false;
    }
}

// Manual control over event state.
void set_opto_flag(enum opto_events event, bool state) {
    switch (event) {
        case FALL:
            opto_fall = state; break;
        case RISE:
            opto_rise = state; break;
        default:
            fprintf(stderr, "Unknown event: %u\n", event);
    }
}
