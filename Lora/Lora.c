#include "Lora.h"

char *get_response()
{
    static char str[DATALEN]; // Make it static to preserve its value between function calls
    int pos = 0;
    while (uart_is_readable_within_us(UART_ID, WAITING_TIME))
    {
        char c = uart_getc(UART_ID);
        if (c == '\n')
        {
            str[pos] = '\0';
            pos = 0; // start over after line is processed
            if (strlen(str))
            {
                return str;
            }
        }
        else
        {
            if (pos < DATALEN - 1)
            {
                str[pos++] = c;
            }
        }
    }
    // If no complete string is received yet, return NULL or an empty string
    return NULL;
}

void send_command(struct LoRaE5 *Lora, char *command)
{
    // Allocate memory for commands
    Lora->commands = (char *)malloc(strlen(command) + 1); // +1 for the null terminator
    strcpy(Lora->commands, command);
    uart_write_blocking(UART_ID, (const uint8_t *)Lora->commands, strlen((char *)Lora->commands));
    // free the allocated memory
    free(Lora->commands);
}

void init_Lora(){
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
}
bool test_lora(struct LoRaE5 *Lora)
{

    send_command(Lora, TEST_LORA);
    char *response = get_response();
    Lora->received_data = (char *)malloc(strlen(response) + 1); // +1 for the null terminator
    strcpy(Lora->received_data, response);
    if (strcmp(Lora->received_data, "+AT: OK\r") == 0)
    {
        free(Lora->received_data);
        return true;
    }
    else
    {
        return false;
    }
}

bool set_mode(struct LoRaE5 *Lora)
{
    send_command(Lora, SET_MODE);
    char *response = get_response();
    Lora->received_data = (char *)malloc(strlen(response) + 1);
    strcpy(Lora->received_data, response);
    if (strcmp(Lora->received_data, "+MODE: LWOTAA\r") == 0)
    {
        free(Lora->received_data);
        return true;
    }
    else
    {
        return false;
    }
}

bool set_key(struct LoRaE5 *Lora)
{
    send_command(Lora, SET_KEY);
    char *response = get_response();
    Lora->received_data = (char *)malloc(strlen(response) + 1);
    strcpy(Lora->received_data, response);
    if (strcmp(Lora->received_data, "+KEY: APPKEY 2B7E151628AED2A6ABF7158809CF4F3C\r") == 0)
    {
        free(Lora->received_data);
        return true;
    }
    else
    {
        return false;
    }
}

bool set_class(struct LoRaE5 *Lora)
{
    send_command(Lora, SET_CLASS);
    char *response = get_response();
    Lora->received_data = (char *)malloc(strlen(response) + 1);
    strcpy(Lora->received_data, response);
    if (strcmp(Lora->received_data, "+CLASS: A\r") == 0)
    {
        free(Lora->received_data);
        return true;
    }
    else
    {
        return false;
    }
}
bool set_port(struct LoRaE5 *Lora)
{
    send_command(Lora, SET_PORT);
    char *response = get_response();
    Lora->received_data = (char *)malloc(strlen(response) + 1);
    strcpy(Lora->received_data, response);
    if (strcmp(Lora->received_data, "+PORT: 8\r") == 0)
    {
        free(Lora->received_data);
        return true;
    }
    else
    {
        return false;
    }
}
bool set_join(struct LoRaE5 *Lora)
{
    char *response;
    send_command(Lora, SET_JOIN);
    for (size_t i = 0; i < 4; i++)
    {
        response = get_response();
        Lora->received_data = (char *)malloc(strlen(response) + 1);
        strcpy(Lora->received_data, response);
        if (strcmp(Lora->received_data, "+JOIN: Join failed\r") == 0 || strcmp(Lora->received_data, "+JOIN: LoRaWAN modem is busy\r") == 0)
        {
            free(Lora->received_data);
            return false;
        }
    }

    if (strcmp(Lora->received_data, "+JOIN: Done\r") == 0)
    {
        free(Lora->received_data);
        return true;
    }
    else
    {
        return false;
    }
}

bool connect_network(struct LoRaE5 *Lora)
{
    if (!set_mode(Lora) || !set_key(Lora) || !set_class(Lora) || !set_port(Lora) || !set_join(Lora))
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool send_message(struct LoRaE5 *Lora, char *data)
{
    char *msg = malloc(strlen(data) + 13);
    sprintf(msg, "AT+MSG=\"%s\"\r\n", data);
    send_command(Lora, msg);
    char *response;

    for (size_t i = 0; i < 7; i++)
    {
        response = get_response();
        Lora->received_data = (char *)malloc(strlen(response) + 1);
        strcpy(Lora->received_data, response);
    }

    if (strcmp(Lora->received_data, "+MSG: Done\r") == 0)
    {
        free(Lora->received_data);
        return true;
    }
    else
    {
        return false;
    }
}
