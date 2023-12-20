#include "../opto-fork/opto.h"
#include "../stepper/stepper.h"
#include "../LED/LED.h"
#include "../switch/switch.h"
#include "../piezo/piezo.h"
#include "../Lora/Lora.h"
#include "../eeprom/eeprom.h"

#define TIME_S ((unsigned long) time_us_64() / 1000000)
#define PILL_INTERVAL_S 10
#define MAX_COMP_COUNT 7 // comp = compartment
#define BLINK_COUNT 5

// LoRa doesn't like \n's
static const char * log_msg[] =
        {"Boot!",
         "LoRa connection established!",
         "LoRa connection failed...",
         "Switch #%u pressed.",
         "Calibrating...",
         "Calibration finished! Full revolution: %d steps.",
         "Calibration finished! Compartment at #%hhu",
         "Dispensing starting from compartment #%hhu",
         "Dispensing finished! Pills detected: %hhu",
         "Rotating to compartment %hhu...",
         "Rotation finished.",
         "Pill found in compartment %d",
         "No pill found in compartment %d, blink lights"};

// used most crucially for logf_msg switch case
enum logs {
    BOOT,
    LORA_1,
    LORA_0,
    SW_PRESSED,
    CALIB_1,
    CALIB_0_1,
    CALIB_0_2,
    DISP_1,
    DISP_0,
    ROT_1,
    ROT_0,
    PILL_1,
    PILL_0,
};

// contains program state information
typedef struct operation_state {
    bool lora_conn; // true if LoRa connection established, false if not
    uint8_t eeprom_log_i; // current free (?) log entry
    uint8_t comp_rotd; // compartments rotated for current dispense ; 7 = default
    uint8_t pills_det; // pills detected during current dispense
} oper_st;

oper_st init_operation();
void print_state(oper_st state);
void logf_msg(enum logs logEnum, oper_st * state, int n_args, ...);
void blink_until_sw_pressed(SW * sw, LED * led);
int rotate_to_event(enum opto_events flag, bool clockwise);
void calibrate(oper_st * state);
void wait_until_sw_pressed(LED * led, SW * sw);
void dispense(oper_st * state, LED * led);