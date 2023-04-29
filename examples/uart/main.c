#include <stdint.h>
#include <stdbool.h>

#include "pll.h"
#include "systick.h"
#include "tm4c123gh6pm.h"

// Note: it wasn't tested with other baudrates/clock speed
#define BAUDRATE 115200
#define CLOCK_SPEED 80000000 // 80 MHz

#define PF2_3 (*((volatile uint32_t *)0x40025030))

void leds_init(void)
{
	SYSCTL_RCGCGPIO_R |= 0x20; // Enable clock to Port F
	while ((SYSCTL_RCGCGPIO_R & 0x20) == 0);

	GPIO_PORTF_DIR_R |= 0x0C;
	GPIO_PORTF_DEN_R |= 0x0C;
}

void led_toggle_blue(void)
{
	PF2_3 = PF2_3 ^ 0x04;
}

void led_toggle_green(void)
{
	PF2_3 = PF2_3 ^ 0x08;
}

// UART0 - microusb port
void uart_init(void)
{
	float bdrd; // baudrate divisor
	uint32_t bdrdf; // baudrate divisor fractional

	SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R0; // activate UART0
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R0; // activate PORT A
	UART0_CTL_R &= ~UART_CTL_UARTEN; // diable UART

	// see page #896 in datasheet for calculation explanation
	bdrd = (float)CLOCK_SPEED / (16 * BAUDRATE);
	bdrdf = (bdrd - (uint32_t)bdrd) * 64 + 0.5;
	UART0_IBRD_R = bdrd; // integer value
	UART0_FBRD_R = bdrdf; // fractional value
	UART0_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_FEN; // 8-bit word length, enable FIFO

	NVIC_PRI1_R = (NVIC_PRI1_R & ~NVIC_PRI1_INT5_M) | (5 << 13); // priority: 5
	NVIC_EN0_R |= 1 << 5; // IRQ 5 enable
	UART0_CTL_R = UART_CTL_RXE | UART_CTL_TXE | UART_CTL_UARTEN; // enable RXE, TXE and UART

	GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & 0xFFFFFF00)
			   | GPIO_PCTL_PA0_U0RX | GPIO_PCTL_PA1_U0TX; // UART
	GPIO_PORTA_AMSEL_R &= ~0x03; // disable analog function on PA1-0
	GPIO_PORTA_AFSEL_R |= 0x03; // enable alt function on PA1-0
	GPIO_PORTA_DEN_R |= 0x03; // enable digital I/O on PA1-0

	UART0_IM_R |= UART_IM_RXIM | UART_IM_RTIM; // Enable Rx/timeout interrupt
}

int i = 0;
char buf[32];

volatile bool rx_semaphore;

void UART0_IntHandler(void)
{
	// Rx timeout or Rx interrupt
	if (UART0_MIS_R & (UART_MIS_RXMIS | UART_MIS_RTMIS)) {
		UART0_ICR_R = UART_ICR_RXIC | UART_ICR_RTIC; // clear Rx/timeout interrupt

		// read until the RX FIFO is empty
		while (!(UART0_FR_R & UART_FR_RXFE)) {
			buf[i++] = UART0_DR_R;
			buf[i] = '\0';
		}

		rx_semaphore = true;
		led_toggle_blue();
	}
}

char uart_inchar(void)
{
	while ((UART0_FR_R & 0x10) != 0); // wait until RXFE is 0
	return ((char)(UART0_DR_R & 0xFF));
}

void uart_putchar(char data)
{
	while ((UART0_FR_R & 0x20) != 0); // block until TXFF is 0
	UART0_DR_R = data;
}

void uart_out_string(char *str)
{
	if (!str)
		return;

	while (*str != '\0' ) {
		if (*str == '\n')
			uart_putchar('\r');

		uart_putchar(*(str++));
	}
}

int main(void)
{
	pll_init_80mhz();
	leds_init();
	uart_init();

	while(true) {
		if (rx_semaphore) {
			UART0_IM_R &= ~(UART_IM_RXIM | UART_IM_RTIM); // Mask Rx interrupt

			uart_out_string(buf);
			i = 0;
			rx_semaphore = false;

			UART0_IM_R |= UART_IM_RXIM | UART_IM_RTIM; // Enable Rx interrupt
		}
	}
}

