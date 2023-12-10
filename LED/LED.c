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

void led_toggle(LED led) {
    gpio_put(led.pin, !check_led_state(&led));
}

void init_pwm(LED led) {
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv_int(&config, DIVIDER);
    pwm_config_set_wrap(&config, WRAP);

    uint slice = pwm_gpio_to_slice_num(led.pin);
    pwm_set_enabled(slice, false);
    pwm_init(slice, &config, false);

    uint channel = pwm_gpio_to_channel(led.pin);
    pwm_set_chan_level(slice, channel, INIT_PWM_LEVEL);
    gpio_set_function(led.pin, GPIO_FUNC_PWM);

    pwm_set_enabled(slice, true);
}

void put_pwm(LED * led, uint16_t level) {
    led->pwm_level = level;
    uint slice = pwm_gpio_to_slice_num(led->pin);
    uint channel = pwm_gpio_to_channel(led->pin);
    pwm_set_chan_level(slice, channel, led->pwm_level);
}

void toggle_pwm(LED * led) {
    led->pwm_level = led->pwm_level == PWM_OFF ? PWM_SOFT : PWM_OFF;
    uint slice = pwm_gpio_to_slice_num(led->pin);
    uint channel = pwm_gpio_to_channel(led->pin);
    pwm_set_chan_level(slice, channel, led->pwm_level);
}
