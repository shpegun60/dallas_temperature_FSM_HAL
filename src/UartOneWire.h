/*
 * UartOneWire.h
 *
 *  Created on: Dec 01, 2021
 *      Author: Shpegun60
 */

#ifndef INC_UART_ONEWIRE_H_
#define INC_UART_ONEWIRE_H_

#include "main.h"

// You can exclude certain features from OneWire.  In theory, this
// might save some space.  In practice, the compiler automatically
// removes unused code (technically, the linker, using -fdata-sections
// and -ffunction-sections when compiling, and Wl,--gc-sections
// when linking), so most of these will not result in any code size
// reduction.  Well, unless you try to use the missing features
// and redesign your program to not need them!  ONEWIRE_CRC8_TABLE
// is the exception, because it selects a fast but large algorithm
// or a small but slow algorithm.

// you can exclude onewire_search by defining that to 0
#ifndef ONEWIRE_SEARCH
#define ONEWIRE_SEARCH 1
#endif

// You can exclude CRC checks altogether by defining this to 0
#ifndef ONEWIRE_CRC
#define ONEWIRE_CRC 1
#endif

// Select the table-lookup method of computing the 8-bit CRC
// by setting this to 1.  The lookup table enlarges code size by
// about 250 bytes.  It does NOT consume RAM (but did in very
// old versions of OneWire).  If you disable this, a slower
// but very compact algorithm is used.
#ifndef ONEWIRE_CRC8_TABLE
//#define ONEWIRE_CRC8_TABLE 1
#endif

// You can allow 16-bit CRC checks by defining this to 1
// (Note that ONEWIRE_CRC must also be 1.)
#ifndef ONEWIRE_CRC16
//#define ONEWIRE_CRC16 1
#endif


#define OW_READ_ATTEMPT		((uint8_t)0x04)

#define OW_RESET_SPEED      ((uint32_t) 9600)
#define OW_TRANSFER_SPEED   ((uint32_t) 115200)

#define OW_0				((uint8_t)0x00)
#define OW_1				((uint8_t)0xff)
#define OW_R_1				((uint8_t)0xff)

#define OW_OK				((uint8_t)0x01)
#define OW_WAIT				((uint8_t)0x02)
#define OW_NO_DEVICE		((uint8_t)0x03)

#define OW_NO_READ			((uint8_t)0xff)
#define OW_READ_SLOT		((uint8_t)0xff)

typedef volatile struct {
	UART_HandleTypeDef* huart;
	unsigned char ROM_RD_NO[8];
	unsigned char ROM_WR_NO[8];
	uint8_t w_size;
	uint8_t r_size;

	uint8_t transmittState;
	uint8_t resetState;
	uint8_t reset_counter;
	uint32_t lastTime;

	uint8_t * command_ptr;
	uint8_t * data_ptr;
	uint8_t read_start;


#if ONEWIRE_SEARCH
	uint8_t search_state;
#endif
} UartOneWire_HandleTypeDef;


HAL_StatusTypeDef OW_Init(UartOneWire_HandleTypeDef* ow, UART_HandleTypeDef* huart);
uint8_t OW_Reset(UartOneWire_HandleTypeDef* ow);
uint8_t OW_Send(UartOneWire_HandleTypeDef* ow, uint8_t *command, uint8_t cLen, uint8_t *data, uint8_t dLen, uint8_t readStart);
void OW_ClearStates(UartOneWire_HandleTypeDef* ow) ;

#if ONEWIRE_SEARCH
uint8_t OW_SearchBlock(UartOneWire_HandleTypeDef* ow, uint8_t *buf, uint8_t num);
#endif


#if ONEWIRE_CRC
// Compute a Dallas Semiconductor 8 bit CRC, these are used in the
// ROM and scratchpad registers.
uint8_t OW_Crc8(const uint8_t *addr, uint8_t len);

#if ONEWIRE_CRC16
// Compute the 1-Wire CRC16 and compare it against the received CRC.
// Example usage (reading a DS2408):
//    // Put everything in a buffer so we can compute the CRC easily.
//    uint8_t buf[13];
//    buf[0] = 0xF0;    // Read PIO Registers
//    buf[1] = 0x88;    // LSB address
//    buf[2] = 0x00;    // MSB address
//    WriteBytes(net, buf, 3);    // Write 3 cmd bytes
//    ReadBytes(net, buf+3, 10);  // Read 6 data bytes, 2 0xFF, 2 CRC16
//    if (!CheckCRC16(buf, 11, &buf[11])) {
//        // Handle error.
//    }
//
// @param input - Array of bytes to checksum.
// @param len - How many bytes to use.
// @param inverted_crc - The two CRC16 bytes in the received data.
//                       This should just point into the received data,
//                       *not* at a 16-bit integer.
// @param crc - The crc starting value (optional)
// @return True, iff the CRC matches.
uint8_t OW_CheckCrc16(const uint8_t* input, uint16_t len, const uint8_t* inverted_crc, uint16_t crc);

// Compute a Dallas Semiconductor 16 bit CRC.  This is required to check
// the integrity of data received from many 1-Wire devices.  Note that the
// CRC computed here is *not* what you'll get from the 1-Wire network,
// for two reasons:
//   1) The CRC is transmitted bitwise inverted.
//   2) Depending on the endian-ness of your processor, the binary
//      representation of the two-byte return value may have a different
//      byte order than the two bytes you get from 1-Wire.
// @param input - Array of bytes to checksum.
// @param len - How many bytes to use.
// @param crc - The crc starting value (optional)
// @return The CRC16, as defined by Dallas Semiconductor.
uint16_t OW_Crc16(const uint8_t* input, uint16_t len, uint16_t crc);
#endif
#endif

#endif /* INC_UART_ONEWIRE_H_ */
