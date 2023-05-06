#include <stdint.h>
#include <stdbool.h>

#include "uart.h"
#include "leds.h"
#include "pll.h"
#include "systick.h"
#include "tm4c123gh6pm.h"
#include "printf.h"
#include "crc8.h"

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

int aht20_softreset(void)
{
	int rc;
	uint8_t cmd[] = {0xBA};

	rc = i2c_transmit(AHT20_I2C_ADDR, cmd, sizeof(cmd));
	if (rc)
		return rc;

	systick_wait_10ms(2);

	return rc;
}

int aht20_init(void)
{
	int rc;
	uint8_t cmd[] = {0xBE, 0x08, 0x00};

	rc = i2c_transmit(AHT20_I2C_ADDR, cmd, sizeof(cmd));
	if (rc)
		return rc;

	systick_wait_10ms(1);

	return rc;
}

int aht20_measure(void)
{
	int rc;
	uint8_t cmd[] = {0xAC, 0x33, 0x00};

	rc = i2c_transmit(AHT20_I2C_ADDR, cmd, sizeof(cmd));
	if (rc)
		return rc;

	systick_wait_10ms(8);

	return rc;
}

int aht20_read_sensor(uint8_t *sensor_data, size_t size)
{
	if (!sensor_data || size < 7)
		return -1;

	return i2c_receive(AHT20_I2C_ADDR, sensor_data, size);
}

int crc_check(uint8_t *sensor_data, size_t size)
{
	uint8_t crc = crc8_calculate(0xFF, sensor_data, size - 1);
	return crc != sensor_data[size - 1];
}

int aht20_calculate(uint8_t *sensor_data, size_t size)
{
	float temp, hum;
	uint32_t traw, hraw;

	if (!sensor_data || size < 7)
		return -1;

	if (crc_check(sensor_data, size)) {
		printf("Crc check failed\n");
		return -1;
	}

	traw = sensor_data[3] & 0x0F;
	traw <<= 8;
	traw |= sensor_data[4];
	traw <<= 8;
	traw |= sensor_data[5];
	temp = ((float)traw * 200 / 0x100000) - 50;

	hraw = sensor_data[1];
	hraw <<= 8;
	hraw |= sensor_data[2];
	hraw <<= 4;
	hraw |= sensor_data[3] >> 4;
	hum = ((float)hraw * 100) / 0x100000;

	printf("temperature: %.1f°C humidity: %.1f%%\n", temp, hum);

	return 0;
}

int main(void)
{
	int rc;
	uint8_t sensor_data[7];

	leds_init_builtin();
	pll_init_80mhz();
	uart_init();
	systick_init();
	i2c_init();

	// CRC: CRC-8-Dallas/Maxim
	crc8_init(0x31);

	printf("\n\nThis is I2C driver for aht20 temperature-humidity sensor\n");
	systick_wait_10ms(4); // give time for aht20 to power up

	rc = aht20_softreset();
	if (rc) {
		printf("Error during aht20 soft reset\n");
		goto exit;
	}

	rc = aht20_init();
	if (rc) {
		printf("Error during aht20 initialization\n");
		goto exit;
	}

	while (true) {
		rc = aht20_measure();
		if (rc) {
			printf("Error during aht20 measurments\n");
			goto next;
		}

		rc = aht20_read_sensor(sensor_data, sizeof(sensor_data));
		if (rc) {
			printf("Error during reading sensor data from aht20\n");
			goto next;
		}

		rc = aht20_calculate(sensor_data, sizeof(sensor_data));
		if (rc)
			printf("Error during aht20 sensor data calculation\n");

next:
		// wait 2s before requesting values again
		systick_wait_10ms(200);
	}

exit:
	while(true) {};
}
