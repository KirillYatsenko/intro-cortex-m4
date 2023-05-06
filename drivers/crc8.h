#ifndef _CRC8_H_
#define _CRC8_H_

#include <stdint.h>

void crc8_init(uint8_t poly);
uint8_t crc8_calculate(uint8_t init, uint8_t const message[], int count);

#endif
