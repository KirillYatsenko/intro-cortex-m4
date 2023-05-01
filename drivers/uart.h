#ifndef _UART_H_
#define _UART_H_

// expects main clock to be set to 80MHz
void uart_init(void);
int uart_put_char(char);
int uart_put_char_poll(char c);
void uart_put_str(char *str);
int uart_get_char(char *);

#endif
