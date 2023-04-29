#include <stdint.h>
#include <stdbool.h>

#include "pll.h"
#include "systick.h"
#include "tm4c123gh6pm.h"

// UART0 - microusb port
void uart_init(void)
{
       SYSCTL_RCGCUART_R |= 0x01; // activate UART0
       SYSCTL_RCGCGPIO_R |= 0x01; // activate PORT A
       UART0_CTL_R &= ~0x01; // diable UART
       UART0_IBRD_R = 43; // baudrate = 115200, see page #896 in datasheet
       UART0_FBRD_R = 26;
       UART0_LCRH_R = 0x70; // 8-bit word length, enable FIFO
       UART0_CTL_R = 0x301; // enable RXE, TXE and UART
       GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & 0xFFFFFF00) + 0x11; // UART
       GPIO_PORTA_AMSEL_R &= ~0x03; // disable analog function on PA1-0
       GPIO_PORTA_AFSEL_R |= 0x03; // enable alt function on PA1-0
       GPIO_PORTA_DEN_R |= 0x03; // enable digital I/O on PA1-0
}

char uart_inchar(void)
{
	while ((UART0_FR_R & 0x10) != 0); // wait until RXFE is 0
	return ((char)(UART0_DR_R & 0xFF));
}

void uart_outchar(char data)
{
	while ((UART0_FR_R & 0x20) != 0); // block until TXFF is 0
	UART0_DR_R = data;
}

void uart_out_string(char *str)
{
	if (!str)
		return;

	while (*str != '\0' )
		uart_outchar(*(str++));
}

int main(void)
{
	pll_init_80mhz();
	systick_init();
	uart_init();

	while (true) {
		systick_wait_10ms(100); // 100 * 10ms = 1s
		uart_out_string("Hi There!");
	}
}

