#ifndef  PIEZO_H
#define PIEZO_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"

void init_piezo();
bool piezo_detection_within_us();
void set_piezo_flag(bool state);
void set_piezo_irq(bool state);

#endif  // PIEZO_H