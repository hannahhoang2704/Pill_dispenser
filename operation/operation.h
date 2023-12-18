#include "../opto-fork/opto.h"
#include "../stepper/stepper.h"
#include "../LED/LED.h"
#include "../switch/switch.h"
#include "../piezo/piezo.h"

#define TIME_S ((unsigned long) time_us_64() / 1000000)
#define PILL_INTERVAL_S 10
#define MAX_SLOT_COUNT 7

static struct state_s {
    uint8_t slots_left;
    uint8_t pills_detected;
} state = {MAX_SLOT_COUNT,
           0};

void init_pins();

void blink_until_sw_pressed(uint sw);

int rotate_to_event(enum opto_events flag, bool clockwise);

void calibrate();

void wait_until_sw_pressed(uint sw);

void dispense();