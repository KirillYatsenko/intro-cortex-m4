#ifndef _SPI_H_
#define _SPI_H_

#include <stdint.h>

/*
 * SSI2 pinout:
 *
 * Fss = PB5 - frame select
 * Clk = PB4 - clock
 * Tx  = PB7 - transmit
 * Rx  = PB6 - receive
 */
void spi_init(uint8_t bus_fq_mhz, uint8_t ssi_clk_mhz);
int spi_write(uint8_t data[], unsigned size);

#endif
