#include <stdlib.h>
#include "Lora.h"

void init_Lora() {
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
}

char *on_uart_rx() {
    static char str[STRLEN_LORA]; // Make it static to preserve its value between function calls
    int pos = 0;
    while (uart_is_readable_within_us(UART_ID, 500000)) {
        char c = uart_getc(UART_ID);
        if (c == '\n') {
            str[pos] = '\0';
            pos = 0; // start over after line is processed
            if (strlen(str)) {
                return str;
            }
        } else {
            if (pos < STRLEN_LORA - 1) {
                str[pos++] = c;
            }
        }
    }
    // If no complete string is received yet, return NULL or an empty string
    return NULL;
}

void start_lora() {
    const uint8_t init_command[] = "AT\r\n";
    char *receivedString;
    for (size_t i = 0; i < 5; i++) {
        uart_write_blocking(UART_ID, init_command, strlen((const char*)init_command));
        receivedString = on_uart_rx();

        if (receivedString != NULL) {
            printf("Connected to LoRa module: %s\n", receivedString);
            break;
        }else{
            printf("Module not responding\n");
        }
    }
}

void set_time() {
    const uint8_t set_time[] = "AT+RTC=, \"2023-12-10 14:26:13\"\r\n";
    uart_write_blocking(UART_ID, set_time, strlen(set_time));
    printf("Time is setted: %s\n", on_uart_rx());
}

void sync_real_time() {
    const uint8_t set_realtime[] = "AT+LW=DTR\r\n";
    uart_write_blocking(UART_ID, set_realtime, strlen(set_realtime));
}

void get_current_time() {
    const uint8_t get_current_time[] = "AT+RTC\r\n";
    uart_write_blocking(UART_ID, get_current_time, strlen(get_current_time));
    printf("Current Time %s\n", on_uart_rx());
}

int get_current_second() {
    const uint8_t get_current_time[] = "AT+RTC\r\n";
    uart_write_blocking(UART_ID, get_current_time, strlen(get_current_time));
    char *receivedString;
    receivedString = on_uart_rx();
    int len = strlen(receivedString);
    if (len >= 2) {
        char lastTwoChars[3];
        lastTwoChars[0] = receivedString[len - 2];
        lastTwoChars[1] = receivedString[len - 1];
        lastTwoChars[2] = '\0';

        // Use atoi to convert the substring to an integer
        return atoi(lastTwoChars);
    } else {
        // Handle the case where the string has fewer than two characters
        return -1; // or any other suitable default value
    }
}

void send_command(enum cmd_enum cmd) {
    //printf("Command: %s", (const char *) commands[cmd]);
    uart_write_blocking(UART_ID,
                        (const uint8_t *) commands[cmd],
                        strlen(commands[cmd]));
}

bool get_cmd_rps(enum cmd_enum cmd, bool loading_bar) {
    char response[STRLEN_LORA];
    while (uart_is_readable_within_us(UART_ID, UART_WAIT_US)) {
        if (loading_bar) printf("#");
        strcpy(response, on_uart_rx());
        if (strcmp(response, fail_rsp[cmd]) == 0) { // could be used for faster failure confirmation
            printf("\nResponse: %s\n", response);
            return false;
        } else if (strcmp(response, success_rsp[cmd]) == 0) {
            return true;
        }
    }
    return false;
}

bool connect_network() {
    bool connecting = true;
    printf("Connecting to LoRa Server...\n"
           "/LOADING\\\n");
    for (enum cmd_enum cmd = MODE; cmd <= JOIN; cmd++) {
        send_command(cmd);
        if (!get_cmd_rps(cmd, true)) {
            fprintf(stderr, "\nLoRa command failed: %s""Connection failed.\n\n", commands[cmd]);
            return false;
        }
    }
    printf("\n");
    return connecting;
}

void send_msg_to_Lora(const char *content, bool wait_for_rsp) {
    char data[STRLEN_LORA];
    sprintf(data, commands[MSG], content);
    uart_write_blocking(UART_ID,
                        (uint8_t *) data,
                        strlen(data));
    if (wait_for_rsp) get_cmd_rps(MSG, false);
}