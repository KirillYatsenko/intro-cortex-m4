#ifndef _UART_H_
#define _UART_H_

// expects main clock to be set to 80MHz
void uart_init(void);
void _putchar(char); // used by printf lib
int uart_get_char(char *);

#endif
