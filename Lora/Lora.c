#include <stdlib.h>
#include "Lora.h"

// initialize Lora UART
void init_Lora() {
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
}

// return a string received from uart within timeout
char *on_uart_rx() {
    static char str[STRLEN_LORA]; // Make it static to preserve its value between function calls
    int pos = 0;
    while (uart_is_readable_within_us(UART_ID, UAR_WAIT_US_FOR_A_STR)) {
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
    return NULL;        // If no complete string is received yet, return NULL or an empty string
}

// send first command 'AT' to check Lora connection
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

// send command to Lora
void send_command(enum cmd_enum cmd) {
    uart_write_blocking(UART_ID,
                        (const uint8_t *) commands[cmd],
                        strlen(commands[cmd]));
}

// Return boolean whether get command response from Lora
bool get_cmd_rsp(enum cmd_enum cmd, bool loading_bar) {
    char response[STRLEN_LORA];
    while (uart_is_readable_within_us(UART_ID, UART_WAIT_US_FOR_RESP)) {
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

// return boolean whether connection with LoRaWAN network is successful or not
bool connect_network() {
    bool connecting = true;
    printf("Connecting to LoRa Server...\n"
           "/LOADING\\\n");
    for (enum cmd_enum cmd = MODE; cmd <= JOIN; cmd++) {
        send_command(cmd);
        if (!get_cmd_rsp(cmd, true)) {
            fprintf(stderr, "\nLoRa command failed: %s""Connection failed.\n\n", commands[cmd]);
            return false;
        }
    }
    printf("\n");
    return connecting;
}

// send message to Lora
void send_msg_to_Lora(const char *content) {
    char data[STRLEN_LORA];
    sprintf(data, commands[MSG], content);
    uart_write_blocking(UART_ID,
                        (uint8_t *) data,
                        strlen(data));
    get_cmd_rsp(MSG, false);
}