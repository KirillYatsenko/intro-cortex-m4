#include <stdint.h>
#include <stddef.h>

#include "i2c.h"
#include "../inc/tm4c123gh6pm.h"

// i2c module #1: PA6(SCL) - PA7(SDA) pins
void i2c_init(uint32_t main_clock_speed, uint32_t i2c_clock)
{
	SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R1; // enable clock to i2c module #1
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R0; // enable clock to GPIO PORTA

	GPIO_PORTA_AFSEL_R |= (1 << 7) | (1 << 6); // set PORTA 6-7 with alternate func
	GPIO_PORTA_ODR_R |= (1 << 7); // set opendrain on SDA
	GPIO_PORTA_DEN_R |= 0xC0; //enable digital I/O on PORTA 6-7
	GPIO_PORTA_PCTL_R |= GPIO_PCTL_PA6_I2C1SCL | GPIO_PCTL_PA7_I2C1SDA; // configure pinmuxing

	I2C1_MCR_R = I2C_MCR_MFE; // set as master device
	I2C1_MTPR_R = (main_clock_speed / (20 * i2c_clock)) - 1; // configure SCL timer period
}

int i2c_transmit(uint8_t addr, uint8_t *val, size_t size)
{
	size_t i;
	volatile uint32_t readback;

	if (!val)
		return -1;

	I2C1_MSA_R = (addr << 1);

	for (i = 0; i < size; i++) {
		I2C1_MDR_R = val[i];

		if (i == 0) {
			if (size == 0)
				I2C1_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_STOP;
			else
				I2C1_MCS_R = I2C_MCS_START | I2C_MCS_RUN;
		} else if (i == size - 1) {
			I2C1_MCS_R = I2C_MCS_RUN | I2C_MCS_STOP;
		} else {
			I2C1_MCS_R = I2C_MCS_RUN;
		}

		// ensure the write buffer is drained before checking BUSY flag
		readback = I2C1_MCS_R;
		while (I2C1_MCS_R & I2C_MCS_BUSY) {};
	}

	return I2C1_MCS_R & I2C_MCS_ERROR;
}

// ToDo: the last byte is not ack
int i2c_receive(uint8_t addr, uint8_t *val, size_t size)
{
	size_t i;
	volatile uint32_t readback;

	if (!val)
		return -1;

	I2C1_MSA_R = (addr << 1) | I2C_MSA_RS; // read from slave

	for (i = 0; i < size; i++) {
		if (i == 0)
			I2C1_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_ACK;
		else if (i == size - 1)
			I2C1_MCS_R = I2C_MCS_RUN | I2C_MCS_STOP;
		else
			I2C1_MCS_R = I2C_MCS_RUN | I2C_MCS_ACK;

		// ensure the write buffer is drained before checking BUSY flag
		readback = I2C1_MCS_R;

		// wait until the transfer is done
		while (I2C1_MCS_R & I2C_MCS_BUSY) {};

		val[i] = I2C1_MDR_R;
	}

	return I2C1_MCS_R & I2C_MCS_ERROR;
}
