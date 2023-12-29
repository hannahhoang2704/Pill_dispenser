#include "../opto-fork/opto.h"
#include "../stepper/stepper.h"
#include "../LED/LED.h"
#include "../switch/switch.h"
#include "../piezo/piezo.h"
#include "../Lora/Lora.h"
#include "../eeprom/eeprom.h"

// Desi-Second, used in get_time_with_decimal
#define TIME_DS (time_us_64() / 100000)
#define DEC_OFFSET 10 // decimal offset
#define SECS_IN_MIN 60 // seconds in a minute
#define SECS_IN_HOUR 3600 // seconds in an hour
#define TIMESTAMP_LEN 9

#define TIME_S (time_us_64() / 1000000)

#define PILL_INTERVAL_S 10
#define PILLCOMP_COUNT 7 // pillcomp = compartments for pills
#define OPTO_OFFSET 10
#define BLINK_COUNT 5



// LoRa doesn't like \n's ; considers them as operators
static const char * log_format[STRLEN_EEPROM - 1 - TIMESTAMP_LEN] =
        {" Boot",
         " LoRa connection established",
         " LoRa connection failed...",
         " Waiting for switch press...",
         " SW_%u pressed",
         " Calibrating...",
         " Calibration finished! Full revolution: %hu steps",
         " Calibration finished! At compartment #%hhu",
         " Dispensing starting from compartment #%hhu",
         " Dispensing finished! Pills detected: %hhu",
         " Rotating to compartment %hhu...",
         " Rotation finished; now at compartment %hhu",
         " Pill found in compartment %hhu",
         " No pill found in compartment %hhu, blink lights"};

// used most crucially for logf_msg switch case
enum logs {
    BOOT,
    LORA_CONNECTED,
    LORA_NOT_CONNECTED,
    WAITING_FOR_SW,
    SW_PRESSED,
    CALIBRATING_STARTED,
    CALIBRATED_FULL,
    CALIBRATED_PARTIAL,
    DISPENSING_STARTED,
    DISPENSING_FINISHED,
    COMPARTMENT_ROTATION_STARTED,
    COMPARTMENT_ROTATION_FINISHED,
    PILL_DETECTED,
    PILL_NOT_DETECTED,
};

// contains program state information
typedef struct operation_state {
    uint8_t eeprom_log_i; // current free log entry
    uint8_t comps_rotd; // compartments rotated for current dispense ; 7 = default
    uint8_t pills_detd; // pills detected during current dispense
    bool lora_conn; // true if LoRa connection established, false if not
} oper_st;

oper_st init_operation();
void print_state(oper_st state);
void logf_msg(enum logs logEnum, oper_st * state, int n_args, ...);
void blink_until_sw_pressed(SW * sw_proceed, LED * led, oper_st * state);
int rotate_to_event(enum opto_events flag, bool clockwise);
void calibrate(oper_st * state);
void wait_until_sw_pressed(SW * sw_proceed, LED * led, oper_st * state);
void dispense(oper_st * state, LED * led);