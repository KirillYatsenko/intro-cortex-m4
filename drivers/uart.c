#include <stdint.h>
#include <stdbool.h>

#include "../inc/tm4c123gh6pm.h"

#include "uart.h"
#include "leds.h"
#include "pll.h"
#include "systick.h"
#include "ring_buffer.h"

// Note: it wasn't tested with other baudrates/clock speed
#define BAUDRATE 115200
#define CLOCK_SPEED 80000000 // 80 MHz
#define BUF_SIZE 8

static struct rg_buf rx_rg_buf;
static struct rg_buf tx_rg_buf;
static char rx_rg_buf_mem[BUF_SIZE];
static char tx_rg_buf_mem[BUF_SIZE];

static void uart_enable_rx_irq(void)
{
	UART0_ICR_R = UART_ICR_RXIC | UART_ICR_RTIC; // clear Rx/timeout interrupt
	UART0_IM_R |= UART_IM_RXIM | UART_IM_RTIM;
}

static void uart_enable_tx_irq(void)
{
	UART0_ICR_R = UART_ICR_TXIC; // clear Tx interrupt
	UART0_IM_R |= UART_IM_TXIM;
}

static void uart_disable_tx_irq(void)
{
	UART0_IM_R &= ~UART_IM_TXIM;
}

// UART0 - microusb port
void uart_init(void)
{
	struct rg_buf_attr rx_buf_attr = { rx_rg_buf_mem, BUF_SIZE };
	struct rg_buf_attr tx_buf_attr = { tx_rg_buf_mem, BUF_SIZE };

	float bdrd; // baudrate divisor
	uint32_t bdrdf; // baudrate divisor fractional

	SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R0; // activate UART0
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R0; // activate PORT A
	UART0_CTL_R &= ~UART_CTL_UARTEN; // diable UART

	// see page #896 in datasheet for calculation explanation
	bdrd = (float)CLOCK_SPEED / (16 * BAUDRATE);
	bdrdf = (bdrd - (uint32_t)bdrd) * 64 + 0.5;
	UART0_IBRD_R = bdrd; // integer value
	UART0_FBRD_R = bdrdf; // fractional value
	UART0_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_FEN; // 8-bit word length, enable FIFO

	NVIC_PRI1_R = (NVIC_PRI1_R & ~NVIC_PRI1_INT5_M) | (5 << 13); // priority: 5
	NVIC_EN0_R |= 1 << 5; // IRQ 5 enable
	UART0_CTL_R = UART_CTL_RXE | UART_CTL_TXE | UART_CTL_UARTEN; // enable RXE, TXE and UART

	GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & 0xFFFFFF00)
			   | GPIO_PCTL_PA0_U0RX | GPIO_PCTL_PA1_U0TX; // UART
	GPIO_PORTA_AMSEL_R &= ~0x03; // disable analog function on PA1-0
	GPIO_PORTA_AFSEL_R |= 0x03; // enable alt function on PA1-0
	GPIO_PORTA_DEN_R |= 0x03; // enable digital I/O on PA1-0

	rg_buf_init(&rx_rg_buf, &rx_buf_attr);
	rg_buf_init(&tx_rg_buf, &tx_buf_attr);

	uart_enable_rx_irq();
	uart_enable_tx_irq();
}

static void start_tx()
{
	char c;

	// fill the hw buffer, if the hw buffer is not full and there is pending
	// data in the sw ring buffer
	while (!(UART0_FR_R & UART_FR_TXFF) && !rg_buf_get_char(&tx_rg_buf, &c))
		UART0_DR_R = c;
}

void UART0_IntHandler(void)
{
	// Rx timeout or Rx interrupt
	if (UART0_MIS_R & (UART_MIS_RXMIS | UART_MIS_RTMIS)) {
		UART0_ICR_R = UART_ICR_RXIC | UART_ICR_RTIC; // clear Rx/timeout interrupt

		// read until the RX FIFO is empty
		while (!(UART0_FR_R & UART_FR_RXFE)) {
			if (rg_buf_put_char(&rx_rg_buf, UART0_DR_R))
				break;
		}
	} else if (UART0_MIS_R & UART_MIS_TXMIS) {  // Tx interrupt
		UART0_ICR_R = UART_ICR_TXIC; // clear Tx interrupt
		start_tx(); // schedule next tx if sw buffer is not empty
	}
}

int uart_put_char_poll(char c)
{
	// wait until hw buffer can receive data
	while ((UART0_FR_R & UART_FR_TXFF) != 0);

	UART0_DR_R = c;

	return 0;
}

int uart_put_char(char c)
{
	bool tx_running;

	// wait for ring buffer to have empty space
	while (rg_buf_is_full(&tx_rg_buf));

	uart_disable_tx_irq();

	tx_running = !rg_buf_is_empty(&tx_rg_buf);
	rg_buf_put_char(&tx_rg_buf, c);

	if (!tx_running)
		start_tx();

	uart_enable_tx_irq();

	return 0;
}

void uart_put_str(char *str)
{
	if (!str)
		return;

	while (*str != '\0' ) {
		if (*str == '\n')
			uart_put_char('\r');

		uart_put_char(*(str++));
	}
}

int uart_get_char(char *c)
{
	return rg_buf_get_char(&rx_rg_buf, c);
}
