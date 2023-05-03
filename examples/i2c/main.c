#include <stdint.h>
#include <stdbool.h>

#include "uart.h"
#include "leds.h"
#include "pll.h"
#include "systick.h"
#include "tm4c123gh6pm.h"
#include "printf.h"

#define CLOCK_SPEED 80000000 // 80 MHz
#define I2C_CLK 100000 // 100 KHz
#define AHT20_I2C_ADDR 0x38

// i2c module #1: PA6(SCL) - PA7(SDA) pins
void i2c_init()
{
	SYSCTL_RCGCI2C_R |= SYSCTL_RCGCI2C_R1; // enable clock to i2c module #1
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R0; // enable clock to GPIO PORTA

	GPIO_PORTA_AFSEL_R |= (1 << 7) | (1 << 6); // set PORTA 6-7 with alternate func
	GPIO_PORTA_ODR_R |= (1 << 7); // set opendrain on SDA
	GPIO_PORTA_DEN_R |= 0xC0;             //enable digital I/O on PORTA 6-7
	GPIO_PORTA_PCTL_R |= GPIO_PCTL_PA6_I2C1SCL | GPIO_PCTL_PA7_I2C1SDA; // configure pinmuxing

	I2C1_MCR_R = I2C_MCR_MFE; // set as master device
	I2C1_MTPR_R = (CLOCK_SPEED / (20 * I2C_CLK)) - 1; // configure SCL timer period
}

int i2c_transmit(uint8_t addr, uint8_t *val, size_t size)
{
	size_t i;
	volatile uint32_t readback;

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

// read one byte
// uint8_t i2c_receive(uint8_t addr, uint8_t *val)
// {
// 	I2C1_MSA_R = (addr << 1) | 0x01; // read from slave
// 	I2C1_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_ACK; // send slave addr
// 
// 	// wait until the transfer is done
// 	while (I2C1_MCS_R & I2C_MCS_BUSY){}
// 
// 	*val = I2C1_MDR_R;
// 
// 	return I2C1_MCS_R & I2C_MCS_ERROR;
// }

int i2c_receive(uint8_t addr, uint8_t *val, size_t size)
{
	volatile uint32_t readback;

	I2C1_MSA_R = (addr << 1) | 0x01; // read from slave
	I2C1_MCS_R = I2C_MCS_START | I2C_MCS_RUN | I2C_MCS_ACK; // send slave addr

	// ensure the write buffer is drained before checking BUSY flag
	readback = I2C1_MCS_R;

	// wait until the transfer is done
	while (I2C1_MCS_R & I2C_MCS_BUSY) {};

	*val = I2C1_MDR_R;

	return I2C1_MCS_R & I2C_MCS_ERROR;
}

int main(void)
{
	int rc;
	uint8_t val_tx[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
	uint8_t rx_tx[2];

	leds_init_builtin();
	pll_init_80mhz();
	uart_init();
	systick_init();
	i2c_init();

	printf("This is i2c driver!\n");
	systick_wait_10ms(100);


	while (true) {
		printf("sizeof: %d\n", sizeof(val_tx));
		rc = i2c_transmit(AHT20_I2C_ADDR, val_tx, sizeof(val_tx));
		if (rc)
			printf("Error during i2c_transmit\n");
		else
			printf("Success i2c_transmit\n");

// 		rc = i2c_receive(AHT20_I2C_ADDR, &val);
// 		if (rc)
// 			printf("Error during i2c_receive\n");
// 		else
// 			printf("Success i2c_receive: 0%b\n", val);


		systick_wait_10ms(10);
	}
}
