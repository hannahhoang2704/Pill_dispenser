// led.c
#include "LED.h"

//void init_led(LED *led) {
//    gpio_init(led->pin);
//    gpio_set_dir(led->pin, GPIO_OUT);
//    gpio_put(led->pin, 0);
//}
//
//bool check_led_state(LED *led) {
//    if (gpio_get(led->pin)) {
//        return 1;
//    } else {
//        return 0;
//    }
//}
//
//void blink_led(LED *led, bool state) {
//    gpio_put(led->pin, state);
//}
//
//void led_toggle(LED led) {
//    gpio_put(led.pin, !check_led_state(&led));
//}

static uint16_t pwm_level = INIT_PWM_LEVEL;

void init_pwm(LED *led) {
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv_int(&config, DIVIDER);
    pwm_config_set_wrap(&config, WRAP);

    led->slice = pwm_gpio_to_slice_num(led->pin);
    pwm_set_enabled(led->slice, false);
    pwm_init(led->slice, &config, false);

    led->channel = pwm_gpio_to_channel(led->pin);
    pwm_set_chan_level(led->slice, led->channel, pwm_level);
    gpio_set_function(led->pin, GPIO_FUNC_PWM);

    pwm_set_enabled(led->slice, true);
}

void set_led_brightness(LED *led, uint16_t level) {
    pwm_level = level;
    pwm_set_chan_level(led->slice, led->channel, pwm_level);
}

void toggle_pwm(LED *led) {
    pwm_level = pwm_level == PWM_OFF ? PWM_SOFT : PWM_OFF;
    pwm_set_chan_level(led->slice, led->channel, pwm_level);
}

void led_blink_times(LED *led, int blink_time) {
    int total_toggle_times = blink_time*2;
    for (int blink = 0; blink < total_toggle_times; ++blink) {
        toggle_pwm(led);
        sleep_ms(200);
    }
}