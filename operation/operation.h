#ifndef OPERATION_H
#define OPERATION_H

#include "../opto-fork/opto.h"
#include "../stepper/stepper.h"
#include "../LED/LED.h"
#include "../switch/switch.h"
#include "../piezo/piezo.h"
#include "../Lora/Lora.h"
#include "../eeprom/eeprom.h"

#define TIME_DS (time_us_64() / 100000) // Desi-Second, used in get_time_with_decimal
#define DEC_OFFSET 10 // decimal offset
#define SECS_IN_MIN 60 // seconds in a minute
#define SECS_IN_HOUR 3600 // seconds in an hour
#define TIMESTAMP_LEN 9

#define TIME_S (time_us_64() / 1000000)

#define PILL_INTERVAL_S 30
#define PILLCOMP_COUNT 7 // pillcomp = compartments for pills
#define OPTO_OFFSET 10
#define BLINK_COUNT 5

enum interrupt_events {
    PIEZO_FALL,
    OPTO_FALL,
    OPTO_RISE
};

// LoRa doesn't like \n's ; considers them as operators
static const char * log_format[STRLEN_EEPROM - 1 - TIMESTAMP_LEN] =
        {" Boot",
         " LoRa connection established",
         " LoRa connection failed...",
         " Press SW_%u to proceed or SW_%u to read logs",
         " SW_%u pressed",
         " Calibrating...",
         " Calibration finished! Full revolution: %hu steps",
         " Recalibration finished! At compartment #%hhu",
         " Dispensing starting from compartment #%hhu",
         " Dispensing finished! Pills detected: %hhu",
         " Rotating to compartment %hhu...",
         " Rotation finished; now at compartment %hhu",
         " Pill found in compartment %hhu",
         " No pill found in compartment %hhu, blink lights"};

// used most crucially for logf_msg switch case
enum logs {
    BOOT,
    LORA_SUCCEED,
    LORA_FAILED,
    WAIT_FOR_SW_PRESSED,
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
    uint8_t eeprom_log_idx; // current free log entry
    uint8_t current_comp_idx; // compartments rotated for current dispense ; 7 = default
    uint8_t pills_detected; // pills detected during current dispense
    bool lora_connected; // true if LoRa connection established, false if not
    bool dispensing_underway;
    LED led;
    SW sw_proceed; // used for proceeding the operation
    SW sw_log; // used for printing logs
} oper_st;

oper_st init_operation();
void set_opto_fork_irq();
void set_piezo_irq();
void logf_msg(enum logs logEnum, oper_st * state, int n_args, ...);
void blink_until_sw_pressed(oper_st * state);
bool piezo_detection_within_us();
int rotate_to_event(enum interrupt_events flag, bool clockwise);
void calibrate(oper_st * state);
void press_sw_to_read_log(oper_st * state);
void wait_until_sw_pressed(oper_st * state);
void dispense(oper_st * state);

#endif //OPERATION_H