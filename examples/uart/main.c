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
	char c;

	pll_init_80mhz();
	systick_init();
	uart_init();

	// Echo input
	while (true) {
		systick_wait_10ms(100); // 100 * 10ms = 1s

		c = uart_inchar();
		uart_out_string("You typed:");
		uart_outchar(c);
	}
}

