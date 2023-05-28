#include <stdint.h>

#include "adc.h"
#include "ring_buffer.h"
#include "../inc/tm4c123gh6pm.h"

#define RG_BUF_SIZE	16

static struct rg_buf rg_buf;
static buf_t tx_rg_buf_mem[RG_BUF_SIZE];

// PE3 is used as ADC0 input
void adc_init(void)
{
	struct rg_buf_attr tx_buf_attr = { tx_rg_buf_mem, RG_BUF_SIZE };
	rg_buf_init(&rg_buf, &tx_buf_attr);

	// configure GPIO PORTE first
	SYSCTL_RCGCADC_R |= SYSCTL_RCGCADC_R0; // enable clock for ADC0
	while ((SYSCTL_RCGCADC_R & SYSCTL_RCGCADC_R0) == 0);

	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4; // enable clock for PORTE
	while ((SYSCTL_RCGCGPIO_R & SYSCTL_RCGCGPIO_R4) == 0);

	GPIO_PORTE_AFSEL_R |= 0x08; // set alternate function for PE3
	GPIO_PORTE_DEN_R &= ~0x08; // clear digital enable bit for PE3
	GPIO_PORTE_AMSEL_R |= 0x08; // disable analog isolation

	// configure ADC0
	ADC0_SSPRI_R = 0x0123; // sequencer 3 has highest priority
	ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN3; // disable sequencer 3 before programming
	ADC0_EMUX_R = ADC_EMUX_EM3_PROCESSOR; // software trigger
	ADC0_SSMUX3_R = ~ADC_SSMUX0_MUX0_M; // select AIN0 as ADC sourc

	// end the sequence before starting again, enable irq status
	ADC0_SSCTL3_R |= ADC_SSCTL3_END0 | ADC_SSCTL3_IE0;

	NVIC_PRI4_R = (NVIC_PRI4_R & ~NVIC_PRI4_INT17_M) | 0x8000; // irq priority 4
	NVIC_EN0_R = 0x20000; // enable IRQ7 for ADC0 SS3
	ADC0_ISC_R |= ADC_ISC_IN3; // clear interrupt first
	ADC0_IM_R |= ADC_IM_MASK3; // enable interrupt for SS3

	ADC0_ACTSS_R |= ADC_ACTSS_ASEN3; // enable sequencer 3
}

void ADC0_SS3_IntHandler(void)
{
	rg_buf_put_data(&rg_buf, ADC0_SSFIFO3_R);
	ADC0_ISC_R |= ADC_ISC_IN3; // clear interrupt
}

uint32_t adc_read()
{
	buf_t adc_val;
	ADC0_PSSI_R |= ADC_PSSI_SS3; // start sequencer 3

	// wait for available data
	while (rg_buf_get_data(&rg_buf, &adc_val));

	return adc_val;
}
