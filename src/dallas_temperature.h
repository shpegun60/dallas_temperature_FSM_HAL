/*
 * dallas_temperature.h
 *
 *  Created on: Dec 01, 2021
 *      Author: Shpegun60
 */

#ifndef INC_DALLAS_TEMPERATURE_H_
#define INC_DALLAS_TEMPERATURE_H_

#include "main.h"
#include "UartOneWire.h"

#define DS_MAX_SENSORS 3

#define DS_AlarmTH  0x64
#define DS_AlarmTL  0x9e

// OneWire commands
#define STARTCONVO      0x44  // Tells device to take a temperature reading and put it on the scratchpad
#define COPYSCRATCH     0x48  // Copy scratchpad to EEPROM
#define READSCRATCH     0xBE  // Read from scratchpad
#define WRITESCRATCH    0x4E  // Write to scratchpad
#define RECALLSCRATCH   0xB8  // Recall from EEPROM to scratchpad
#define READPOWERSUPPLY 0xB4  // Determine if device needs parasite power
#define ALARMSEARCH     0xEC  // Query bus for devices with an alarm condition
#define SkipROM 	((uint8_t)0xCC) // skip ROM
#define SearchROM 	((uint8_t)0xF0) // Search ROM
#define ReadROM 	((uint8_t)0x33) // Read ROM
#define MatchROM 	((uint8_t)0x55) // match ROM
#define AlarmROM 	((uint8_t)0xEC) // Alarm ROM

// Scratchpad locations
#define TEMP_LSB        0
#define TEMP_MSB        1
#define HIGH_ALARM_TEMP 2
#define LOW_ALARM_TEMP  3
#define CONFIGURATION   4
#define INTERNAL_BYTE   5
#define COUNT_REMAIN    6
#define COUNT_PER_C     7
#define SCRATCHPAD_CRC  8

// DSROM FIELDS
#define DSROM_FAMILY    0
#define DSROM_CRC       7

// Device resolution
#define TEMP_9_BIT  0x1F //  9 bit
#define TEMP_10_BIT 0x3F // 10 bit
#define TEMP_11_BIT 0x5F // 11 bit
#define TEMP_12_BIT 0x7F // 12 bit

typedef struct {
	UartOneWire_HandleTypeDef* ow;
	// count of devices on the bus
	uint8_t devicesCount;
	float temp[DS_MAX_SENSORS];
	uint8_t id[8 * DS_MAX_SENSORS];

	uint8_t rddata[10];
	uint8_t wrdata[20];

	uint8_t state;
	uint8_t resolution;
	uint32_t lastTime;
	uint8_t counteRead;
} DallasTemperatureData;

void DT_SetOneWire(DallasTemperatureData* dt, UartOneWire_HandleTypeDef* ow);
uint8_t DT_Search(DallasTemperatureData* dt);
void DT_init(DallasTemperatureData* dt, uint8_t resolution);
uint8_t DT_ContiniousProceed(DallasTemperatureData* dt, uint32_t time);

// getters--------------------------------------------------------------------
float getTemperatureByPosition_Celsius(DallasTemperatureData* dt, uint8_t position);


#endif /* INC_DALLAS_TEMPERATURE_H_ */
