#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

#define SYSDIV2 4
#define PF2 (*((volatile uint32_t *)0x40025010))

void led_init(void)
{
	SYSCTL_RCGCGPIO_R |= 0x20; // Enable clock to Port F
	while ((SYSCTL_RCGCGPIO_R & 0x20) == 0);

	GPIO_PORTF_DIR_R |= 0x04;
	GPIO_PORTF_DEN_R |= 0x04;
}

void led_toggle(void)
{
	PF2 = PF2 ^ 0x04;
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

int main(void)
{
	pll_init_80mhz();
	systick_init();
	led_init();

	while (true) {
		systick_wait_10ms(100); // 100 * 10ms = 1s
		led_toggle();
	}
}

