#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

#define SYSDIV2 4
#define PF2 (*((volatile uint32_t *)0x40025010))

void pll_init_80mhz(void)
{
	SYSCTL_RCC2_R |= 0x80000000; // USERCC2
	SYSCTL_RCC2_R |= 0x00000800; // BYPASS2, PLL bypass
	SYSCTL_RCC_R = (SYSCTL_RCC_R & ~0x7C0) // clear bits 10-6
			+ 0x540; // 10101, configure for 16MHz crystal
	SYSCTL_RCC2_R &= ~0x70; // configure for main oscillator source
	SYSCTL_RCC2_R &= ~0x2000; // activate PLL by clearing PWRDN
	SYSCTL_RCC2_R |= 0x40000000; // use 400 MHz PLL
	SYSCTL_RCC2_R = (SYSCTL_RCC2_R & ~0x1FC00000) + (SYSDIV2 << 22); // 80MHz

	while ((SYSCTL_RIS_R & 0x40) == 0); // wait for the PLL to lock, PLLLRIS

	SYSCTL_RCC2_R &= ~0x800; // enable use of PLL by clearing BYPASS
}

void systick_wait(uint32_t delay)
{
	NVIC_ST_RELOAD_R = delay - 1;
	NVIC_ST_CURRENT_R = 0; // any value written to CURRENT clears
	while ((NVIC_ST_CTRL_R & 0x10000) == 0); // waits for COUNT flag
}

void systick_wait_10ms(uint32_t delay)
{
	uint32_t i;
	for (i = 0; i < delay; i++) {
		/* expects 80MHz source clock
		 * 1/80Mhz = 12.5 ns
		 * 12.5ns * 800000 = 10ms
		 */
		systick_wait(800000); // wait 10ms
	}
}

void systick_init(void)
{
	NVIC_ST_CTRL_R = 0; // disable systick
	NVIC_ST_RELOAD_R = 0x00FFFFFF; // maximum reload value 2^24
	NVIC_ST_CURRENT_R = 0; // any write to current will clear it
	NVIC_ST_CTRL_R = 0x05; // enable systick with core clock
}

// UART3 is enabled
// Rx - PC6
// Tx - PC7
void uart_init(void)
{
	SYSCTL_RCGCUART_R |= 0x08; // activate UART3
	SYSCTL_RCGCGPIO_R |= 0x04; // activate PORT C
	UART3_CTL_R &= ~0x01; // diable UART
	UART3_IBRD_R = 43; // baudrate = 115200, see page #896 in datasheet
	UART3_FBRD_R = 26;
	UART3_LCRH_R = 0x70; // 8-bit word length, enable FIFO
	UART3_CTL_R = 0x301; // enable RXE, TXE and UART
	GPIO_PORTC_PCTL_R = (GPIO_PORTC_PCTL_R & 0x00FFFFFF) | 0x11000000; // UART
	GPIO_PORTC_AMSEL_R &= ~0xC0; // disable analog function on PC7-6
	GPIO_PORTC_AFSEL_R |= 0xC0; // enable alt function on PC7-6
	GPIO_PORTC_DEN_R |= 0xC0; // enable digital I/O on PC7-6
}

char uart_inchar(void)
{
	while ((UART3_FR_R & 0x10) != 0); // wait until RXFE is 0
	return ((char)(UART3_DR_R & 0xFF));
}

void uart_outchar(char data)
{
	while ((UART3_FR_R & 0x20) != 0); // block until TXFF is 0
	UART3_DR_R = data;
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

