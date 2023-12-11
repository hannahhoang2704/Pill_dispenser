#include "BTN.h"

void init_button(Button *btn) {
    gpio_init(btn->pin);
    gpio_set_dir(btn->pin, GPIO_IN);
    gpio_pull_up(btn->pin);
    btn->state = gpio_get(btn->pin);
}

int is_button_clicked(Button *btn) {

    if (gpio_get(btn->pin) == 0 && btn->state == 0) {
        btn->state = 1;
        return true;
    } else if (gpio_get(btn->pin) == 1 && btn->state == 1) {
        btn->state = 0;
    }
    return false;
}