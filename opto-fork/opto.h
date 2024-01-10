#include "pico/stdlib.h"

#ifndef OPTO_H
#define OPTO_H

#define OPTO_GPIO 28
#define EVENT_DEBOUNCE_US 5000

void init_opto_fork();
#endif