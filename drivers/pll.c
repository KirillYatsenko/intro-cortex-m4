#include <stdint.h>
#include "pll.h"

#include "../inc/tm4c123gh6pm.h"

#define SYSDIV2 4

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
