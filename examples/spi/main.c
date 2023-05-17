#include <stdint.h>
#include <stdbool.h>

#include "systick.h"
#include "uart.h"
#include "pll.h"
#include "tm4c123gh6pm.h"
#include "printf.h"
#include "ring_buffer.h"
#include "leds.h"

#define ESC			0x1B
#define MAIN_CLK_FQ_MHZ		80
#define SSI_CLK_FQ_MHZ		4
#define RG_BUF_SIZE		128

static struct rg_buf tx_rg_buf;
static char tx_rg_buf_mem[RG_BUF_SIZE];

extern void EnableInterrupts(void);

void spi_write(uint8_t data)
{
	while((SSI2_SR_R & SSI_SR_TNF) == 0){};

	SSI2_DR_R = data;
}

void spi_disable_tx_irq(void)
{
	SSI2_IM_R &= ~SSI_IM_TXIM;
}

void spi_enable_tx_irq(void)
{
	SSI2_IM_R |= SSI_IM_TXIM;
}

void spi_start_tx(void)
{
	char c;

	// tx is already started and in progress
	if ((SSI2_SR_R & SSI_SR_TFE) == 0)
		return;

	spi_disable_tx_irq(); // enter critical section

	while ((SSI2_SR_R & SSI_SR_TNF) && !rg_buf_is_empty(&tx_rg_buf)) {
		rg_buf_get_char(&tx_rg_buf, &c);
		SSI2_DR_R = c;
	}

	if (!rg_buf_is_empty(&tx_rg_buf))
		spi_enable_tx_irq(); // schedule next tx
}

void SSI2_IntHandler(void)
{
	if (SSI2_MIS_R & SSI_MIS_TXMIS)
		spi_start_tx();
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

	// SSI2 interrupt = 57
	NVIC_PRI14_R = (NVIC_PRI14_R & ~NVIC_PRI14_INTB_M) | 0x8000; // priority 4
	NVIC_EN1_R |= 0x02000000; // enable interrupt #57
	spi_disable_tx_irq(); // make sure the irq is disabled

	SSI2_CR1_R = SSI_CR1_SSE; // enable SSI2
}

int main(void)
{
	uint8_t i = 0;
	struct rg_buf_attr tx_buf_attr = { tx_rg_buf_mem, RG_BUF_SIZE };

	pll_init_80mhz();
	uart_init();
	systick_init();
	leds_init_builtin();

	rg_buf_init(&tx_rg_buf, &tx_buf_attr);
	spi_init(MAIN_CLK_FQ_MHZ, SSI_CLK_FQ_MHZ);

	while (true) {
		for (i = 0; i < RG_BUF_SIZE; i++)
			rg_buf_put_char(&tx_rg_buf, i);

		spi_start_tx();
		systick_wait_10ms(1);
	}
}
