#include <stdlib.h>
#include "command.h"
#include "opto.h"
#include "stepper.h"

#define INPUT_TIMEOUT 50000

st valid_command_received(char *input, int *octates) {
    static const int max_input = MAX_CMD_LEN + EXCESS_C;
    static int input_i = 0;

    int input_c;
    while ((input_c = getchar_timeout_us(INPUT_TIMEOUT)) != PICO_ERROR_TIMEOUT) {
        if (input_i == 0) printf("\n>>");
        if ((char) input_c != INPUT_TERMINATOR) {
            printf("%c", (char) input_c);
            if (input_i < max_input)
                input[input_i++] = input_c;
        } else {
            printf("\n");
            input[input_i] = '\0';
            input_i = 0;
            size_t input_len = strlen(input);
            if (strcmp(input, "calib") == 0) {
                return CALIBRATE;
            } else if (strcmp(input, "status") == 0) {
                return STATUS;
            } else {
                char command[MAX_CMD_LEN + NULLC];
                int excess = -1;
                switch (sscanf(input, "%3s %2d%d", command, octates, &excess)) {
                    case 1:
                        *octates = 0;
                    case 2:
                        if (strcmp(command, "run") == 0) {
                            strcpy(input, command);
                            return RUN;
                        }
                    default:
                        fprintf(stderr, "Invalid command: [%s%s]\n",
                                input,
                                input_len > MAX_CMD_LEN ?
                                "..." : "");
                }
            }
        }
    }
    return WAIT;
}

#define STEP_SHIFT_MIN_US 800
#define STEP_SHIFT_INIT_US (STEP_SHIFT_MIN_US + 2000)

#define CALIB_OPERATIONS 4
#define OPTO_OFFSET 158

int calibrate() {
    uint64_t step_shift = STEP_SHIFT_INIT_US;

    printf("Calibrating...\n");

    const bool operation_order[CALIB_OPERATIONS] = {false,
                                                    true,
                                                    false,
                                                    true};
    int steps = 0;

    for (int oper_i = 0; oper_i < CALIB_OPERATIONS; oper_i++) {
        bool condition = operation_order[oper_i];
        while (opto_high() == condition) {
            step(true);
            if (oper_i > 1) ++steps;
            sleep_us(step_shift);
            step_shift = step_shift > STEP_SHIFT_MIN_US ?
                         step_shift - 1 :
                         STEP_SHIFT_MIN_US;
        }
    }

    for (int s = 0; s < OPTO_OFFSET; s++) {
        step(true);
        sleep_us(step_shift);
        step_shift = step_shift < STEP_SHIFT_INIT_US ?
                     step_shift + 20 :
                     STEP_SHIFT_INIT_US;
    }

    printf("%d steps for full revolution.\n", steps);

    return steps;
}

void print_status(int steps) {
    printf("Calibrated: %s\nRevolution: ",
           steps != -1 ? "YES" : "NO");
    if (steps != -1)
        printf("%d\n", steps);
    else
        printf("Unknown\n");
}

#define THEORETICAL_REV 4096

void rotate_octets(int full_rev, int octets) {
    full_rev = full_rev == -1 ? THEORETICAL_REV : full_rev;
    int steps_per_octet = full_rev / 8;
    bool direction = octets >= 0;
    int target = octets != 0 ?
                 steps_per_octet * abs(octets) :
                 full_rev;

    uint64_t step_shift = STEP_SHIFT_INIT_US;
    for (int step_i = 0; step_i < target; step_i++) {
        step(direction);
        sleep_us(step_shift);
        step_shift = step_shift > STEP_SHIFT_MIN_US ?
                     step_shift - 1 :
                     STEP_SHIFT_MIN_US;
    }
}