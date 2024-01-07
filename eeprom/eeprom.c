#include <stdio.h>
#include <string.h>
#include "hardware/i2c.h"
#include "eeprom.h"

//initialize i2c0 pins
void init_eeprom() {
    i2c_init(i2c0, BAUDRATE);
    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);
}

//write an array of data to eeprom
void write_to_eeprom(uint16_t memory_address, const uint8_t *data, size_t length) {
    uint8_t buf[ADDR_LEN + length];
    buf[0] = (uint8_t)(memory_address >> 8);    //high byte of memory address
    buf[1] = (uint8_t)(memory_address);         //low byte of memory address
    for(size_t i = 0; i < length; ++i) {
        buf[i + ADDR_LEN] = data[i];
    }
    i2c_write_blocking(i2c0, DEVADDR, buf, length + ADDR_LEN, false);
    sleep_ms(WRITE_CYCLE_TIME_PER_BYTE * (length + ADDR_LEN));
}

//read an array of data from eeprom
void read_from_eeprom(uint16_t memory_address, uint8_t *data_read, size_t length) {
    uint8_t buf[ADDR_LEN + length];
    buf[0] = (uint8_t)(memory_address >> 8); //high byte of memory address
    buf[1] = (uint8_t)(memory_address);     //low byte of memory address
    i2c_write_blocking(i2c0, DEVADDR, buf, ADDR_LEN, true);
    i2c_read_blocking(i2c0, DEVADDR, data_read, length, false);
}

//Returns a value data stored in eeprom
uint8_t get_stored_value(uint16_t memory_address) {
    uint8_t value;
    read_from_eeprom(memory_address, &value, 1);
    return value;
}

//cyclic redundancy check to detect error log in eeprom memory
uint16_t crc16(const uint8_t *data_p, size_t length) {
    uint8_t x;
    uint16_t crc = 0xFFFF;
    while (length--) {
        x = crc >> 8 ^ *data_p++;
        x ^= x >> 4;
        crc = (crc << 8) ^ ((uint16_t) (x << 5) ^ ((uint16_t) x));
    }
    return crc;
}

// write a string log to eeprom. If log entries reach maximum, the log is erased before write to eeprom
void write_log_entry(const char *str, uint8_t *index) {
    if (*index >= MAX_ENTRIES) {
        printf("Maximum log entries. Erasing the log to log the messages\n");
        erase_logs(index);
    }
    size_t size_length = strlen(str); //not include NULL terminator

    if (size_length > STRLEN_EEPROM - 1) {
        size_length = STRLEN_EEPROM - 1;
    }
    uint8_t log_buf[size_length + 3];

    //copy string to uint8_t array
    for (int a = 0; a < strlen(str); ++a) {
        log_buf[a] = (uint8_t) str[a];
    }
    log_buf[strlen(str)] = '\0';

    //add CRC to log buffer
    uint16_t crc = crc16(log_buf, size_length + 1);
    log_buf[size_length + 1] = (uint8_t)(crc >> 8);
    log_buf[size_length + 2] = (uint8_t)crc;         //check again the size length

    //write to EEPROM
    uint16_t write_address = (uint16_t) FIRST_ADDRESS + (*index * (uint16_t) ENTRY_SIZE);
    if (write_address < ENTRY_SIZE * MAX_ENTRIES) {
        write_to_eeprom(write_address, log_buf, ENTRY_SIZE);
        *index += 1;
        write_to_eeprom(LOG_INDEX_ADDR, index, 1);
    }
}

// read all the log entries that are valid from eeprom using crc to check
void read_log_entry(uint8_t index) {
    printf("Reading log entry\n");
    for (int i = 0; i < index; ++i) {
        uint8_t read_buff[ENTRY_SIZE];
        uint16_t read_address = (uint16_t) FIRST_ADDRESS + (i * (uint16_t) ENTRY_SIZE);
        read_from_eeprom(read_address, (uint8_t *) &read_buff, ENTRY_SIZE);
//        printf("Raw data from EEPROM at index %d\n", i);
//        for (int k = 0; k < ENTRY_SIZE; ++k){
//            printf("%02X", read_buff[k]);
//        }
//        printf("\n");
        int term_zero_index = 0;
        while (read_buff[term_zero_index] != '\0') {
            term_zero_index++;
        }

        if (read_buff[0] != 0 && crc16(read_buff, (term_zero_index + 3)) == 0 && term_zero_index < (ENTRY_SIZE - 2)) {
            printf("Log entry index %d: ", i);
            int b_index = 0;
            while (read_buff[b_index]) {
                printf("%c", read_buff[b_index++]);
            }
            printf("\n");
        } else {
            printf("Invalid or empty log entry at index %d\n", i);
            break; // Stop if an invalid entry is encountered
        }
    }
    printf("\nStop\n");
}

// erase all log entries in eeprom
void erase_logs(uint8_t *log_index) {
    printf("Erase the log messages\nDelete log from address:\n");
    for (int i = 0; i < MAX_ENTRIES; ++i) {
        uint16_t write_address = FIRST_ADDRESS + (i * ENTRY_SIZE);
        printf("%u  ", write_address);
        uint8_t buf[] = {00};
        write_to_eeprom(write_address, buf, 1);
    }
    *log_index = 0;
    printf("\n");
}