// led.c
#include "LED.h"

void init_led(LED *led) {
    gpio_init(led->pin);
    gpio_set_dir(led->pin, GPIO_OUT);
    gpio_put(led->pin, 0);
}

bool check_led_state(LED *led) {
    if (gpio_get(led->pin)) {
        return 1;
    } else {
        return 0;
    }
}

void blink_led(LED *led, bool state) {
    gpio_put(led->pin, state);
}

void led_toggle(LED led) {
    gpio_put(led.pin, !check_led_state(&led));
}

static uint16_t pwm_level = INIT_PWM_LEVEL;

void init_pwm() {
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv_int(&config, DIVIDER);
    pwm_config_set_wrap(&config, WRAP);

    uint slice = pwm_gpio_to_slice_num(LED_3);
    pwm_set_enabled(slice, false);
    pwm_init(slice, &config, false);

    uint channel = pwm_gpio_to_channel(LED_3);
    pwm_set_chan_level(slice, channel, pwm_level);
    gpio_set_function(LED_3, GPIO_FUNC_PWM);

    pwm_set_enabled(slice, true);
}

void put_pwm(uint16_t level) {
    pwm_level = level;

    uint slice = pwm_gpio_to_slice_num(LED_3);
    uint channel = pwm_gpio_to_channel(LED_3);
    pwm_set_chan_level(slice, channel, pwm_level);
}

void toggle_pwm() {
    pwm_level = pwm_level == PWM_OFF ? PWM_SOFT : PWM_OFF;

    uint slice = pwm_gpio_to_slice_num(LED_3);
    uint channel = pwm_gpio_to_channel(LED_3);
    pwm_set_chan_level(slice, channel, pwm_level);
}

void led_blink_times(int blink_time) {
    for (int blink = 0; blink < blink_time; ++blink) {
        toggle_pwm();
        sleep_ms(200);
        toggle_pwm();
        sleep_ms(200);
    }
    put_pwm(PWM_OFF);
}