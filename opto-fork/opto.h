#include "pico/stdlib.h"

#ifndef OPTO_H
#define OPTO_H

enum opto_flags {
    FALL,
    RISE
};

bool opto_flag_state(uint8_t flag);

void set_opto_flag(uint8_t flag, bool state);

void init_opto_fork_irq();

#endif