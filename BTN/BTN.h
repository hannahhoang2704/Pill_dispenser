#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// button.h
#ifndef BTN_H
#define BTN_H

#include "pico/stdlib.h"

typedef struct {
    uint pin;
    uint state;
} Button;

void init_button(Button *btn);
int is_button_clicked(Button *btn);

#endif  // BTN_H