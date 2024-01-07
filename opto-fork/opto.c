#include "opto.h"

// Initialize opto-fork.
void init_opto_fork() {
    gpio_init(OPTO_GPIO);
    gpio_pull_up(OPTO_GPIO);
}