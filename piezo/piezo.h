#ifndef  PIEZO_H
#define PIEZO_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"

//volatile bool pillDetected = false;
extern volatile bool piezo_falling_edge;

void init_piezo();
bool piezo_detection_within_us();

#endif  // PIEZO_H