#ifndef UART_H
#define UART_H

#include "hardware/uart.h"
#include "pico/stdlib.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define UART_ID uart0
#define STRLEN 80
#define WAITING_TIME 10000 // 10 seconds
#define TEST_LORA "AT\r\n"
#define SET_MODE "AT+MODE=LWOTAA\r\n"
#define SET_KEY "AT+KEY=APPKEY,\"2B7E151628AED2A6ABF7158809CF4F3C\"\r\n"
#define SET_CLASS "AT+CLASS=A\r\n"
#define SET_PORT "AT+PORT=8\r\n"
#define SET_JOIN "AT+JOIN\r\n"

struct LoRaE5
{
    char *commands;
    char *received_data;
};

bool test_lora(struct LoRaE5 *Lora);
bool connect_network(struct LoRaE5 *Lora);
bool send_message(struct LoRaE5 *Lora, char *data);

#endif