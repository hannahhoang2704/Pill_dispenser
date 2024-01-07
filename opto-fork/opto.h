#include "pico/stdlib.h"

#ifndef OPTO_H
#define OPTO_H

#define OPTO_GPIO 28
#define EVENT_DEBOUNCE_US 5000

enum opto_events {
    FALL,
    RISE
};

bool opto_flag_state(enum opto_events event);
void set_opto_flag(enum opto_events event, bool state);
void init_opto_fork();
#endif