#ifndef LED_H
#define LED_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

#define LED_1 22
#define LED_2 21
#define LED_3 20

#define DIVIDER 125
#define WRAP 999

#define PWM_OFF 0
#define PWM_SOFT 50 // 5%
#define INIT_PWM_LEVEL PWM_OFF

typedef struct {
    uint pin;
    uint slice;
    uint channel;
    uint16_t pwm_level;
} LED;

LED init_pwm(uint led_pin);
void set_led_brightness(LED *led, uint16_t level);
void toggle_pwm(LED *led);
void led_blink_times(LED *led, int blink_time);

#endif  // LED_H