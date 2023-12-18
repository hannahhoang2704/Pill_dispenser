#include "Lora.h"

void init_Lora(){
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
}

char* on_uart_rx() {
    static char str[STRLEN]; // Make it static to preserve its value between function calls
    int pos = 0;
    while (uart_is_readable_within_us(UART_ID, 500000)) {
        char c = uart_getc(UART_ID);
        if ( c == '\n') {
            str[pos] = '\0';
            pos = 0; // start over after line is processed
            if(strlen(str))
            {
                return str;                
            }
        }else 
        {
            if (pos < STRLEN - 1) {
                str[pos++] = c;
            }
        }
    }
    // If no complete string is received yet, return NULL or an empty string
    return NULL;
}
void start_lora(){
    const uint8_t init_command[] = "AT\r\n";
    char* receivedString;
            for (size_t i = 0; i < 5; i++)
                {
                    uart_write_blocking(UART_ID, init_command, strlen(init_command));
                    receivedString = on_uart_rx();

                    if (receivedString != NULL) 
                    {
                        printf("Connected to LoRa module: %s\n", receivedString);
                        break;
                    }
                    if (receivedString == NULL )
                    {
                        printf("Module not responding\n");
                    }  
                }
}

void set_time(){
    const uint8_t set_time [] = "AT+RTC=, \"2023-12-10 14:26:13\"\r\n";
    uart_write_blocking(UART_ID, set_time, strlen(set_time));
    printf("Time is setted: %s\n", on_uart_rx());
}

void sync_real_time(){
    const uint8_t set_realtime[] = "AT+LW=DTR\r\n";
    uart_write_blocking(UART_ID, set_realtime, strlen(set_realtime));
}

void get_current_time(){
    const uint8_t get_current_time[] = "AT+RTC\r\n";
    uart_write_blocking(UART_ID, get_current_time, strlen(get_current_time));
    printf("Current Time %s\n", on_uart_rx());
}
int get_current_second(){
    const uint8_t get_current_time[] = "AT+RTC\r\n";
    uart_write_blocking(UART_ID, get_current_time, strlen(get_current_time));
    char* receivedString;
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

void send_command(const uint8_t * command) {
    uart_write_blocking(UART_ID,
                        command,
                        strlen((char *) command));

    printf("Sending: %s", (char *) command);

    uint64_t time_us = time_us_64();
    printf("[rsp_time us] *response*\n");
    while (uart_is_readable_within_us(UART_ID, UART_WAIT_US)) {
        printf("[%llu us] %s\n",
               time_us_64() - time_us,
               on_uart_rx());
        time_us = time_us_64();
    }
    printf("\n");
}

void connect_network(){
    const uint8_t set_mode [] = "AT+MODE=LWOTAA\r\n";
    const uint8_t set_key [] = "AT+KEY=APPKEY,\"3ffb2c845fe93f3f5a99c91c11844b81\"\r\n";
    const uint8_t set_class [] = "AT+CLASS=A\r\n";
    const uint8_t set_port [] = "AT+PORT=8\r\n";
    const uint8_t set_join [] = "AT+JOIN\r\n";
    const uint8_t dev_eui [] = "AT+BEACON=INFO\r\n";

    send_command(set_mode);
    send_command(set_key);
    send_command(set_class);
    send_command(set_port);
    send_command(set_join);
}

void send_msg(){
    const uint8_t data[] = "AT+MSG=\"Hello Phuong\"\r\n";

    send_command(data);
}