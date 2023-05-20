#ifndef _NOKIA5110_H_
#define _NOKIA5110_H_

/*  Wiring
 * PA7 - D/C
 * PA6 - RST
 * PB5 - SCE
 * PB4 - SCLK
 * PB7 - MOSI
 */
void nokia5110_init(void);

// expects null terminated string
void nokia5110_write_str(char *str);

#endif
