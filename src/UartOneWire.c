/*
 * UartOneWire.c
 *
 *  Created on: Dec 01, 2021
 *      Author: Shpegun60
 */
#include "UartOneWire.h"


static HAL_StatusTypeDef OW_UART_Init(UartOneWire_HandleTypeDef* ow, uint32_t baudRate)
{
	UART_HandleTypeDef* HUARTx = ow->huart;
	HUARTx->Init.BaudRate = baudRate;
	HUARTx->Init.WordLength = UART_WORDLENGTH_8B;
	HUARTx->Init.StopBits = UART_STOPBITS_1;
	HUARTx->Init.Parity = UART_PARITY_NONE;
	HUARTx->Init.Mode = UART_MODE_TX_RX;
	HUARTx->Init.HwFlowCtl = UART_HWCONTROL_NONE;
	HUARTx->Init.OverSampling = UART_OVERSAMPLING_16;
	return HAL_UART_Init(HUARTx);
}

HAL_StatusTypeDef OW_Init(UartOneWire_HandleTypeDef* ow, UART_HandleTypeDef* huart)
{
	if(!ow) {
		return HAL_ERROR;
	}
	ow->huart = huart;
	ow->transmittState = 0;
	ow->resetState = 0;
	ow->reset_counter = 0;
	ow->lastTime = 0;
	return OW_UART_Init(ow, OW_RESET_SPEED);
}

void OW_ClearStates(UartOneWire_HandleTypeDef* ow) {
	if(!ow) {
		return;
	}
	ow->transmittState = 0;
	ow->resetState = 0;
	ow->reset_counter = 0;
	ow->lastTime = 0;
}

static void OW_ToBits(uint8_t owByte, uint8_t *owBits)
{
	for (uint8_t i = 0; i < 8; ++i) {
		*owBits = (owByte & 0x01) ? OW_1 : OW_0;
		owBits++;
		owByte = owByte >> 1;
	}
}

static uint8_t OW_ToByte(uint8_t *owBits) {
	uint8_t owByte = 0;

	for (uint8_t i = 0; i < 8; ++i) {
		owByte = owByte >> 1;

		if (*owBits == OW_R_1) {
			owByte |= 0x80;
		}
		owBits++;
	}

	return owByte;
}

// Perform the onewire reset function.  We will wait up to 250uS for
// the bus to come high, if it doesn't then it is broken or shorted

uint8_t OW_Reset(UartOneWire_HandleTypeDef* ow)
{
	switch(ow->resetState) {

	case 0:
		OW_UART_Init(ow, OW_RESET_SPEED);
		ow->reset_counter = OW_READ_ATTEMPT;
		ow->resetState++;
		break;

	case 1:
		ow->ROM_WR_NO[0] = (uint8_t)0xf0;
		HAL_UART_Receive_DMA(ow->huart, (uint8_t*)ow->ROM_RD_NO, 1);
		HAL_UART_Transmit_DMA(ow->huart, (uint8_t*)ow->ROM_WR_NO, 1);
		ow->lastTime = HAL_GetTick();
		ow->resetState++;
		break;

	case 2: {
		if((HAL_UART_GetState(ow->huart) == HAL_UART_STATE_READY) || ((HAL_GetTick() - ow->lastTime) > 5)) {
			ow->resetState++;
		}
		break;
	}

	case 3:
		if (ow->ROM_RD_NO[0] != 0xf0) {
			ow->resetState++;
		} else if(ow->reset_counter) {
			ow->reset_counter--;
			ow->resetState = 1;
		} else {
			ow->resetState++;
		}

		break;

	case 4:
		OW_UART_Init(ow, OW_TRANSFER_SPEED);
		ow->resetState = 0;
		return ow->reset_counter ? OW_OK : OW_NO_DEVICE;
		break;

	default:
		ow->resetState = 0;
		break;
	}
	return OW_WAIT;
}


//-----------------------------------------------------------------------------
// procedure bus communication 1-wire
// command - an array of bytes sent to the bus. If you need reading, we send OW_READ_SLOT
// cLen - the length of the command buffer, as many bytes will be sent to the bus
// data - if reading is required, then a reference to the buffer for reading
// dLen - the length of the read buffer. Read no more than this length
// readStart - which transmission character to start reading from (numbered from 0)
// you can specify OW_NO_READ, then you don't need to specify data and dLen
//-----------------------------------------------------------------------------
uint8_t OW_Send(UartOneWire_HandleTypeDef* ow, uint8_t *command, uint8_t cLen, uint8_t *data, uint8_t dLen, uint8_t readStart)
{
	switch(ow->transmittState) {

	case 0: {
		uint8_t state = OW_Reset(ow);

		if(state == OW_NO_DEVICE) {
			return OW_NO_DEVICE;
		} else if(state == OW_OK) {
			ow->command_ptr = command;
			ow->w_size = cLen;

			ow->data_ptr = data;
			ow->r_size = dLen;
			ow->read_start = readStart;


			ow->transmittState++;
		}
		break;
	}

	case 1:
		if(ow->w_size) {
			OW_ToBits(*ow->command_ptr, (uint8_t *)ow->ROM_WR_NO);
			ow->command_ptr++;
			ow->w_size--;

			HAL_UART_Receive_DMA(ow->huart, (uint8_t*)ow->ROM_RD_NO, 8);
			HAL_UART_Transmit_DMA(ow->huart, (uint8_t*)ow->ROM_WR_NO, 8);
			ow->lastTime = HAL_GetTick();
			ow->transmittState++;
		} else {
			ow->transmittState = 0;
			return OW_OK;
		}
		break;

	case 2:
		if (HAL_UART_GetState(ow->huart) == HAL_UART_STATE_READY || ((HAL_GetTick() - ow->lastTime) > 5)) {
			ow->transmittState++;
		}
		break;

	case 3:
		if (!ow->read_start && ow->r_size) {
			*ow->data_ptr = OW_ToByte((uint8_t *)ow->ROM_RD_NO);
			ow->data_ptr++;
			ow->r_size--;
		} else {
			if (ow->read_start != OW_NO_READ) {
				ow->read_start--;
			}
		}
		ow->transmittState = 1;
		break;

	default:
		ow->transmittState = 0;
		break;
	}

	return OW_WAIT;
}

#if ONEWIRE_SEARCH

static void OW_SendBits(UartOneWire_HandleTypeDef* ow, uint8_t numBits)
{
	HAL_UART_Receive_DMA(ow->huart, (uint8_t*)ow->ROM_RD_NO, numBits);
	HAL_UART_Transmit_DMA(ow->huart, (uint8_t*)ow->ROM_WR_NO, numBits);

	while (HAL_UART_GetState(ow->huart) != HAL_UART_STATE_READY) {
		__NOP();
	}
}

uint8_t OW_SearchBlock(UartOneWire_HandleTypeDef* ow, uint8_t *buf, uint8_t num)
{
	uint8_t searchRomCMD = (uint8_t)0xf0;
	uint8_t found = 0;
	uint8_t state = 0;
	uint8_t *lastDevice = NULL;
	uint8_t *curDevice = buf;
	uint8_t numBit, lastCollision, currentCollision, currentSelection;

	ow->transmittState = 0;
	ow->resetState = 0;
	HAL_UART_DMAStop(ow->huart);

	lastCollision = 0;

	while (found < num) {
		numBit = 1;
		currentCollision = 0;

		while(1) {
			state = OW_Send(ow, &searchRomCMD, 1, NULL, 0, OW_NO_READ);
			if(state != OW_WAIT) {
				break;
			}
		}

		for (numBit = 1; numBit <= 64; numBit++) {
			OW_ToBits(OW_READ_SLOT, (uint8_t*)ow->ROM_WR_NO);
			OW_SendBits(ow, 2);

			if (ow->ROM_RD_NO[0] == OW_R_1) {
				if (ow->ROM_RD_NO[1] == OW_R_1) {
					return found;
				} else {
					currentSelection = 1;
				}
			} else {
				if (ow->ROM_RD_NO[1] == OW_R_1) {
					currentSelection = 0;
				} else {
					if (numBit < lastCollision) {
						if (lastDevice[(numBit - 1) >> 3] & 1 << ((numBit - 1) & 0x07)) {
							currentSelection = 1;

							if (currentCollision < numBit) {
								currentCollision = numBit;
							}
						} else {
							currentSelection = 0;
						}
					} else {
						if (numBit == lastCollision) {
							currentSelection = 0;
						} else {
							currentSelection = 1;

							if (currentCollision < numBit) {
								currentCollision = numBit;
							}
						}
					}
				}
			}

			if (currentSelection == 1) {
				curDevice[(numBit - 1) >> 3] |= 1 << ((numBit - 1) & 0x07);
				OW_ToBits(0x01, (uint8_t*)ow->ROM_WR_NO);
			} else {
				curDevice[(numBit - 1) >> 3] &= ~(1 << ((numBit - 1) & 0x07));
				OW_ToBits(0x00, (uint8_t*)ow->ROM_WR_NO);
			}
			OW_SendBits(ow, 1);
		}

		found++;
		lastDevice = curDevice;
		curDevice += 8;
		if (currentCollision == 0) {
			return found;
		}

		lastCollision = currentCollision;
	}
	return found;
}

#endif


#if ONEWIRE_CRC
// The 1-Wire CRC scheme is described in Maxim Application Note 27:
// "Understanding and Using Cyclic Redundancy Checks with Maxim iButton Products"
//

#if ONEWIRE_CRC8_TABLE
// Dow-CRC using polynomial X^8 + X^5 + X^4 + X^0
// Tiny 2x16 entry CRC table created by Arjen Lentz
// See http://lentz.com.au/blog/calculating-crc-with-a-tiny-32-entry-lookup-table
static const uint8_t dscrc2x16_table[] = {
		0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83,
		0xC2, 0x9C, 0x7E, 0x20, 0xA3, 0xFD, 0x1F, 0x41,
		0x00, 0x9D, 0x23, 0xBE, 0x46, 0xDB, 0x65, 0xF8,
		0x8C, 0x11, 0xAF, 0x32, 0xCA, 0x57, 0xE9, 0x74
};

// Compute a Dallas Semiconductor 8 bit CRC. These show up in the ROM
// and the registers.  (Use tiny 2x16 entry CRC table)
uint8_t OW_Crc8(const uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;

	while (len--)
	{
		crc = *addr++ ^ crc;  // just re-using crc as intermediate
		crc = dscrc2x16_table[crc & 0x0f] ^ dscrc2x16_table[16 + ((crc >> 4) & 0x0f)];
	}

	return crc;
}
#else
//
// Compute a Dallas Semiconductor 8 bit CRC directly.
// this is much slower, but a little smaller, than the lookup table.
//
uint8_t OW_Crc8(const uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;

	while (len--)
	{
		uint8_t inbyte = *addr++;
		for (uint8_t i = 8; i; i--)
		{
			uint8_t mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix) crc ^= 0x8C;
			inbyte >>= 1;
		}
	}
	return crc;
}
#endif

#if ONEWIRE_CRC16

uint16_t OW_Crc16(const uint8_t* input, uint16_t len, uint16_t crc)
{
	static const uint8_t oddparity[16] = { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };

	for (uint16_t i = 0 ; i < len ; i++)
	{
		// Even though we're just copying a byte from the input,
		// we'll be doing 16-bit computation with it.
		uint16_t cdata = input[i];
		cdata = (cdata ^ crc) & 0xff;
		crc >>= 8;

		if (oddparity[cdata & 0x0F] ^ oddparity[cdata >> 4])
			crc ^= 0xC001;

		cdata <<= 6;
		crc ^= cdata;
		cdata <<= 1;
		crc ^= cdata;
	}

	return crc;
}

uint8_t OW_CheckCrc16(const uint8_t* input, uint16_t len, const uint8_t* inverted_crc, uint16_t crc)
{
	crc = ~OW_Crc16(input, len, crc);
	return (crc & 0xFF) == inverted_crc[0] && (crc >> 8) == inverted_crc[1];
}

#endif

#endif

