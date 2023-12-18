#include <pico/printf.h>
#include "hardware/i2c.h"
#include "pico/stdlib.h"

#include "operation/operation.h"

#define TIME_S ((unsigned long) time_us_64() / 1000000)
#define PILL_INTERVAL_S 10
#define PILL_COUNT 7
#define BLINK_TIMES 5

int main() {
    stdio_init_all();

    LED led_3 = {.pin = LED_3};
    init_pwm(&led_3);

    init_switch(SW_0);

    init_opto_fork();
    init_stepper();
    init_piezo();

    while (true) {

        blink_until_sw_pressed(SW_0);

        calibrate();

        wait_until_sw_pressed(SW_0);

        dispense();
    }
}