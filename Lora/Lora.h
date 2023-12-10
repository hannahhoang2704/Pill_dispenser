
#ifndef UART_H
#define UART_H

#include "hardware/uart.h"
#include "pico/stdlib.h"
#include <string.h>
#include <stdio.h>

#define UART_ID uart0
#define BAUD_RATE 9600
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE
#define STRLEN 80

void UART_init();
char* on_uart_rx();
void init_lora();
void set_time();
void sync_real_time();
void get_current_time();
int get_current_second();

#endif
