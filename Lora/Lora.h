
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
#define STRLEN_LORA 80
#define UART_WAIT_US_FOR_RESP 10000000
#define UAR_WAIT_US_FOR_A_STR 500000

static const char * commands[] =
        {"AT\r\n",
         "AT+MODE=LWOTAA\r\n",
//         "AT+KEY=APPKEY,\"3ffb2c845fe93f3f5a99c91c11844b81\"\r\n", //change your own KEY //roni's key
         "AT+KEY=APPKEY,\"d3f7167dea8b2f45da7fc8dfcf0ff6e6\"\r\n", //change your own KEY  //hannah's key
         "AT+CLASS=A\r\n",
         "AT+PORT=8\r\n",
         "AT+JOIN\r\n",
         "AT+MSG=\"%s\"\r\n"};

static const char * success_rsp[] =
        {"+AT: OK\r",
         "+MODE: LWOTAA\r",
//         "+KEY: APPKEY 3FFB2C845FE93F3F5A99C91C11844B81\r", //change your own KEY     //Roni's key
         "+KEY: APPKEY D3F7167DEA8B2F45DA7FC8DFCF0FF6E6\r", //change your own KEY //Hannah's key
         "+CLASS: A\r",
         "+PORT: 8\r",
         "+JOIN: Done\r",
         "+MSG: Done\r"};

static const char * fail_rsp[] =
        {"unknown\r",
         "unknown\r",
         "unknown\r",
         "unknown\r",
         "unknown\r",
         "+JOIN: Join failed\r",
         "+MSG: Please join network first\r"};

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
bool connect_network();
void send_msg_to_Lora(const char *content);

#endif
