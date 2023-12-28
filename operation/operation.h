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

// LoRa doesn't like \n's ; considers them as operators
static const char * log_msg[] =
        {" Boot",
         " LoRa connection established",
         " LoRa connection failed...",
         " Switch #%u pressed",
         " Calibrating...",
         " Calibration finished! Full revolution: %d steps",
         " Calibration finished! At compartment #%hhu",
         " Dispensing starting from compartment #%hhu",
         " Dispensing finished! Pills detected: %hhu",
         " Rotating to compartment %hhu...",
         " Rotation finished",
         " Pill found in compartment %d",
         " No pill found in compartment %d, blink lights"};

// used most crucially for logf_msg switch case
enum logs {
    BOOT,
    LORA_SUCCEED,
    LORA_FAILED,
    SW_PRESSED,
    CALIB_START,
    CALIB_COMPLETED,
    RECALIB_AFTER_REBOOT,
    DISPENSE_CONTINUED,
    DISPENSE_COMPLETED,
    ROTATION_CONTINUED,
    ROTATION_COMPLETED,
    PILL_FOUND,
    NO_PILL_FOUND
};

// contains program state information
typedef struct operation_state {
    uint8_t eeprom_log_idx; // current free (?) log entry
    uint8_t current_comp_idx; // compartments rotated for current dispense ; 7 = default
    uint8_t pills_detected; // pills detected during current dispense
    bool lora_connected; // true if LoRa connection established, false if not
} oper_st;

oper_st init_operation();
void print_state(oper_st state);
void logf_msg(enum logs logEnum, oper_st * state, int n_args, ...);
void blink_until_sw_pressed(SW * sw_proceed, LED * led);
int rotate_to_event(enum opto_events flag, bool clockwise);
void calibrate(oper_st * state);
void wait_until_sw_pressed(SW * sw_proceed, LED * led);
void dispense(oper_st * state, LED * led);