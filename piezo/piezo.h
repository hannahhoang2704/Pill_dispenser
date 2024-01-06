#ifndef  PIEZO_H
#define PIEZO_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define PIEZO_SENSOR 27
#define PIEZO_MAX_WAITING_TIME 85000


void init_piezo();
bool piezo_detection_within_us();
void set_piezo_flag(bool state);
void set_piezo_irq(bool state);

#endif  // PIEZO_H