#include <stdint.h>
#include <stdbool.h>

#include "nokia5110.h"
#include "systick.h"
#include "pll.h"
#include "tm4c123gh6pm.h"
#include "spi.h"
#include "printf.h"
#include "uart.h"

// PE3 is used as ADC0 input
void adc_init(void)
{
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
					    //
	// end the sequence before starting again, enable irq
	ADC0_SSCTL3_R |= ADC_SSCTL3_END0 | ADC_SSCTL3_IE0;

	ADC0_IM_R &= ~ADC_IM_MASK3; // disable interrupt for SS3
	ADC0_ACTSS_R |= ADC_ACTSS_ASEN3; // enable sequencer 3
}

uint32_t adc_read(void)
{
	ADC0_PSSI_R |= ADC_PSSI_SS3; // start sequencer 3

	while ((ADC0_RIS_R &= ADC_RIS_INR3) == 0) {};

	ADC0_ISC_R |= ADC_ISC_IN3; // clear interrupt

	return ADC0_SSFIFO3_R;
}

int main(void)
{
	float conversion = 3.3 / 4096;

	pll_init_80mhz();
	systick_init();
	adc_init();
	nokia5110_init();
	uart_init();

	while (true) {
		nokia5110_clear_row(2);
		nokia5110_set_col(6);

		fctprintf(nokia5110_put_char, NULL, "%.2f V",
			  adc_read() * conversion);

		printf("%.2f V\n", adc_read() * conversion);

		systick_wait_10ms(10); // wait 500ms
	}
}
