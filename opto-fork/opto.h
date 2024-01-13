#ifndef OPTO_H
#define OPTO_H

#include "pico/stdlib.h"

#define OPTO_GPIO 28
#define EVENT_DEBOUNCE_US 5000

void init_opto_fork();
#endif