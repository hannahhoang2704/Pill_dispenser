// led.c
#include "LED.h"

void init_led(LED *led) {
    gpio_init(led->pin);
    gpio_set_dir(led->pin, GPIO_OUT);
    gpio_put(led->pin, 0);
}

bool check_led_state(LED *led){
    if(gpio_get(led->pin))
    {
        return 1;
    }else{
        return 0;
    }
}

void blink_led(LED *led, bool state) {
    gpio_put(led->pin, state);
}