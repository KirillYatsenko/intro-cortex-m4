#include <stdint.h>
#include <stdbool.h>

#include "systick.h"
#include "uart.h"
#include "pll.h"
#include "tm4c123gh6pm.h"
#include "printf.h"

#define ESC			0x1B
#define MAIN_CLK_FQ_MHZ		80
#define SSI_CLK_FQ_MHZ		4

extern void EnableInterrupts(void);

void timer_init(uint32_t period_us, uint8_t bus_fq_mhz)
{
	SYSCTL_RCGCTIMER_R |= 0x01; // enable timer0 clock
	while ((SYSCTL_RCGCTIMER_R & 0x01) == 0);

	TIMER0_CTL_R = 0; // disable the timer
	TIMER0_CFG_R = 0; // configure for 32-bit mode

	TIMER0_TAMR_R = 0x02; // periodic mode
	TIMER0_TAILR_R = (period_us * bus_fq_mhz) - 1;  // reload value
	TIMER0_ICR_R = 0x01; // clear timer0 timeout flag
	TIMER0_IMR_R |= 0x01; // enable timeout interrupt
	NVIC_PRI4_R = (NVIC_PRI4_R & 0x0FFFFFFF) | 0x40000000; // priority 2
	NVIC_EN0_R |= 0x80000; // enable irq 19
	TIMER0_CTL_R |= 0x01; // enable timer

	EnableInterrupts();
}

void spi_write(uint8_t data)
{
	while((SSI2_SR_R & SSI_SR_TNF) == 0){};

	SSI2_DR_R = data;
}

void Timer0_IntHandler(void)
{
	static int i = 0;

	TIMER0_ICR_R = 0x01; // clear timeout interrupt flag

	spi_write(i++);
}

/*
 * SSI2 pinout:
 *
 * Fss = PB5 - frame select
 * Clk = PB4 - clock
 * Tx  = PB7 - transmit
 * Rx  = PB6 - receive
 */
void spi_init(uint8_t bus_fq_mhz, uint8_t ssi_clk_mhz)
{
	SYSCTL_RCGCSSI_R |= SYSCTL_RCGCSSI_R2; // enable clock to SSI2 module
	while ((SYSCTL_RCGCSSI_R & SYSCTL_RCGCSSI_R2) == 0);

	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1; // enable clock to PORTB
	while ((SYSCTL_RCGCGPIO_R & SYSCTL_RCGCGPIO_R1) == 0);

	GPIO_PORTB_AFSEL_R |= 0xF0; // enable alternate func on pins PB7-4
	GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R & ~0xFFFF0000) | 0x22220000; // configure pinmux
	GPIO_PORTB_DEN_R |= 0xF0; // digital enable on pins PB7-4

	SSI2_CR1_R = ~SSI_CR1_SSE; // make sure the SSI is disabled before configuring
	SSI2_CR1_R = ~SSI_CR1_MS; // configure SSI as master device
	SSI2_CC_R = SSI_CC_CS_SYSPLL; // set system clock as the source to SSI

	// SSI Clk = SysClk / (CPSDVSR * ( 1 + SCR ))
	SSI2_CPSR_R = bus_fq_mhz / ssi_clk_mhz;
	SSI2_CR0_R = (SSI2_CR0_R & ~SSI_CR0_FRF_M) | SSI_CR0_DSS_8; // Freescale SPI, 8 bit data width

	SSI2_CR1_R = SSI_CR1_SSE; // enable SSI2
}

int main(void)
{
	pll_init_80mhz();
	uart_init();
	systick_init();

	timer_init(1000000 * 1, 80); // 1s

	spi_init(MAIN_CLK_FQ_MHZ, SSI_CLK_FQ_MHZ);

	while (true) { }
}
