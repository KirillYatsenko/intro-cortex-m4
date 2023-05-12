#include <stdint.h>
#include <stdbool.h>

#include "uart.h"
#include "leds.h"
#include "pll.h"
#include "systick.h"
#include "tm4c123gh6pm.h"
#include "printf.h"
#include "crc8.h"
#include "i2c.h"

#define CLOCK_SPEED 80000000 // 80 MHz
#define I2C_CLK 100000 // 100 KHz
#define CRC_POLY 0x31 // CRC: CRC-8-Dallas/Maxim
#define CRC_INIT 0xFF

#define AHT20_I2C_ADDR 0x38

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
	uint8_t crc = crc8_calculate(CRC_INIT, sensor_data, size - 1);
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
	i2c_init(CLOCK_SPEED, I2C_CLK);

	crc8_init(CRC_POLY);

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
