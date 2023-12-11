// led.h
#ifndef LED_H
#define LED_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"

typedef struct {
    uint pin;
} LED;

void init_led(LED *led);
bool check_led_state(LED *led);
void blink_led(LED *led, bool state);

#endif  // LED_H
