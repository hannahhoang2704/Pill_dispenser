
#ifndef UART_H
#define UART_H

#include "hardware/uart.h"
#include "pico/stdlib.h"
#include <string.h>
#include <stdio.h>

#define UART_ID uart1
#define BAUD_RATE 9600
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE
#define STRLEN 80
#define UART_WAIT_US 10000000

#define WAIT_FOR_MSG_RSP true // usually takes several seconds

static const char * commands[] =
        {"AT\r\n",
         "AT+MODE=LWOTAA\r\n",
         "AT+KEY=APPKEY,\"3ffb2c845fe93f3f5a99c91c11844b81\"\r\n",
         "AT+CLASS=A\r\n",
         "AT+PORT=8\r\n",
         "AT+JOIN\r\n",
         "AT+MSG=\"%s\"\r\n"};

static const char * succ_rsp[] =
        {"+AT: OK\r",
         "+MODE: LWOTAA\r",
         "+KEY: APPKEY 3FFB2C845FE93F3F5A99C91C11844B81\r",
         "+CLASS: A\r",
         "+PORT: 8\r",
         "+JOIN: Done\r",
         "+MSG: Done\r"};

// we could search for failure responses aswell ;
// didn't do yet, because documentation doesn't mention for all commands

enum cmd_enum {
    AT,
    MODE,
    APPKEY,
    CLASS,
    PORT,
    JOIN,
    MSG
};

void init_Lora();
char* on_uart_rx();
void start_lora();
void set_time();
void sync_real_time();
void get_current_time();
int get_current_second();
bool connect_network();
void send_msg(const char *content);

#endif
