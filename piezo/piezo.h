#ifndef  PIEZO_H
#define PIEZO_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"

void init_piezo();
bool piezo_detection_within_us();

#endif  // PIEZO_H