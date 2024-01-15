# Pill Dispenser

1.	Introduction
The goal of the project is to develop an automated pill dispensing system that delivers medication daily to users at a specific time. The main components and peripherals of this project include a microcontroller board Raspberry Pi Pico, a dispenser base, a dispenser wheel, equipped with eight compartments (seven for pills and one for calibration) which is operated by a stepper motor. A piezoelectric sensor is also integrated, for detecting whether a pill is dropped out from the compartment and an optical sensor (opto-fork) is for wheel positioning and calibration. During the program’s execution, the program's state including a total number of detected pills and dispensing state as well as descriptive log messages are stored in EEPROM I2C, allowing for operations beyond reboots. Additionally, some device status is sent to the server via the LoRaWAN network and also logged to stdout via a Debug Probe. In terms of software, the project utilizes Embedded C programming within the Raspberry Pi Pico SDK framework.

This documentation provides an overview of the program workflow, implementation principles as well as in-depth peripherals features and utilities within the scope of the project. 

