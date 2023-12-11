#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define SW_0 9
#define SW_1 8
#define SW_2 7

typedef struct {
    uint pin;
    bool pressed;
} SW;

void init_switch(SW sw);

bool switch_pressed(SW sw);

bool is_button_clicked(SW * sw);