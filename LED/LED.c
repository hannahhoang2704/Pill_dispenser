#include "LED.h"

void init_pwm(LED *led) {
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv_int(&config, DIVIDER);
    pwm_config_set_wrap(&config, WRAP);

    led->slice = pwm_gpio_to_slice_num(led->pin);
    pwm_set_enabled(led->slice, false);
    pwm_init(led->slice, &config, false);

    led->channel = pwm_gpio_to_channel(led->pin);
    pwm_set_chan_level(led->slice, led->channel, INIT_PWM_LEVEL);
    gpio_set_function(led->pin, GPIO_FUNC_PWM);

    pwm_set_enabled(led->slice, true);
}

void set_led_brightness(LED *led, uint16_t level) {
    led->pwm_level = level;
    pwm_set_chan_level(led->slice, led->channel, led->pwm_level);
}

void toggle_pwm(LED *led) {
    led->pwm_level = led->pwm_level == PWM_OFF ? PWM_SOFT : PWM_OFF;
    pwm_set_chan_level(led->slice, led->channel, led->pwm_level);
}

void led_blink_times(LED *led, int blink_time) {
    int total_toggle_times = blink_time*2;
    for (int blink = 0; blink < total_toggle_times; ++blink) {
        toggle_pwm(led);
        sleep_ms(200);
    }
}