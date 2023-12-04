#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"

// 'run XXX' = 7 for a negative 2 digit int
#define MAX_CMD_LEN 6
#define EXCESS_C 1
#define NULLC 1
#define INPUT_TERMINATOR '\r'

typedef enum state_enum {
    WAIT,
    CALIBRATE,
    STATUS,
    RUN
} st;

st valid_command_received(char *input, int *octets);

int calibrate();

void print_status(int steps);

void rotate_octets(int full_rev, int octets);