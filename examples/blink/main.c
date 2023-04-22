#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

#define SYSDIV2 4
#define PF2 (*((volatile uint32_t *)0x40025010))
#define PD0 (*((volatile uint32_t *)0x40007004)) // Used for profiling

static volatile uint32_t systick_irq_counter;

extern void EnableInterrupts(void);

void led_init(void)
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

void led_toggle(void)
{
	PF2 = PF2 ^ 0x04;
	PD0 = PD0 ^ 0x01;
}

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

void systick_init(uint32_t counter)
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

