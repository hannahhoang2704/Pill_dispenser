#include "pico/stdlib.h"

#ifndef OPTO_H
#define OPTO_H

// volatile bool opto_falling_edge = false;

void unflag_opto_event();

void init_opto_fork();

void init_opto_fork_irq();

bool opto_high();

#endif