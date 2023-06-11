#include <stdint.h>
#include <stdbool.h>

#include "pll.h"
#include "tm4c123gh6pm.h"

#define SYSDIV2 4
#define PF2 (*((volatile uint32_t *)0x40025010))
#define PD0 (*((volatile uint32_t *)0x40007004)) // Used for profiling

static volatile uint32_t systick_irq_counter;

extern void EnableInterrupts(void);

static void led_init(void)
{
	SYSCTL_RCGCGPIO_R |= 0x28; // Enable clock to Port F and Port D
	while ((SYSCTL_RCGCGPIO_R & 0x28) == 0);

	GPIO_PORTF_DIR_R |= 0x04;
	GPIO_PORTF_DEN_R |= 0x04;

	GPIO_PORTD_AMSEL_R &= ~0x01;
	GPIO_PORTD_PCTL_R &= ~0x0F;
	GPIO_PORTD_DIR_R |= 0x01;
	GPIO_PORTD_AFSEL_R &= ~0x01;
	GPIO_PORTD_DEN_R |= 0x01;
}

static void led_toggle(void)
{
	PF2 = PF2 ^ 0x04;
	PD0 = PD0 ^ 0x01;
}

static void systick_init(uint32_t counter)
{
	systick_irq_counter = 0;

	NVIC_ST_CTRL_R = 0; // disable systick
	NVIC_ST_RELOAD_R = counter - 1;
	NVIC_ST_CURRENT_R = 0; // any write to current will clear it
	NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R & 0x00FFFFFF) | 0x40000000; // priority 2
	NVIC_ST_CTRL_R = 0x07; // enable systick with core clock and interrupts

	EnableInterrupts();
}

void SysTickHandler(void)
{
	if (systick_irq_counter == 4) {
		// since systick is configured to request irq each 200ms
		// (4 + 1(current inc)) * 200ms = 1s
		systick_irq_counter = 0;
		led_toggle();
	} else {
		systick_irq_counter++;
	}
}

int main(void)
{
	led_init();
	pll_init_80mhz();

	/* expects 80MHz main clock which is configured above
	 * period = 1s / 80MHz = 12.5ns
	 * counter = 200ms / 12.5ns = 16000000
	 */
	systick_init(16000000);

	while (true);
}

