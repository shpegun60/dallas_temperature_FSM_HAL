## Nonblocking continious ds18b20 handler, one wire over uart with dma

library included autosearch for devices on one bus by collisions after initialization

```c
#include "dallas_temperature.h"
#include "UartOneWire.h"

extern UART_HandleTypeDef huart;
UartOneWire_HandleTypeDef ow;
DallasTemperatureData dt;

uint8_t resolution = TEMP_12_BIT;

int main () {

	OW_Init(&ow, &huart);
	DT_SetOneWire(&dt, &ow);
	DT_init(&dt, resolution);
	
	while(1) {
		uint32_t millis = HAL_GetTick();
		DT_ContiniousProceed(&dt, millis);	
	}
}
```
for get temperature and id devices go to  "DallasTemperatureData" declaration
