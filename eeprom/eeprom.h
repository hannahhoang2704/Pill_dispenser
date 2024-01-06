//
// Created by Hanh Hoang on 15.12.2023.
//

#ifndef PILL_DISPENSER_EEPROM_H
#define PILL_DISPENSER_EEPROM_H

#include "pico/stdlib.h"

#define I2C0_SDA_PIN 16
#define I2C0_SCL_PIN 17

#define DEVADDR 0x50
#define BAUDRATE 100000
#define I2C_MEMORY_SIZE 32768
#define WRITE_CYCLE_TIME_PER_BYTE 5

#define ENTRY_SIZE 64
#define MAX_ENTRIES 32
#define STRLEN 62
#define FIRST_ADDRESS 0

void initialize_eeprom();
void write_to_eeprom(uint16_t memory_address, uint8_t *data, size_t length);
void read_from_eeprom(uint16_t memory_address, uint8_t *data_read, size_t length);
uint16_t crc16(const uint8_t *data_p, size_t length);
void write_log_entry(char *str, uint8_t *index, uint16_t index_address);
void read_log_entry(uint8_t *index);
void erase_logs(uint8_t *current_index, uint16_t index_address);

#endif //PILL_DISPENSER_EEPROM_H
