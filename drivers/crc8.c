/*
 * Implementation taken from: https://barrgroup.com/embedded-systems/how-to/crc-calculation-c-code
 */

#include <stdint.h>
#include "crc8.h"

/*
 * The width of the CRC calculation and result.
 * Modify the typedef for a 16 or 32-bit CRC standard.
 */

#define WIDTH  (8 * sizeof(uint8_t))
#define TOPBIT (1 << (WIDTH - 1))

uint8_t  crcTable[256];

void crc8_init(uint8_t poly)
{
    uint8_t  remainder;

    /*
     * Compute the remainder of each possible dividend.
     */
    for (int dividend = 0; dividend < 256; ++dividend)
    {
        /*
         * Start with the dividend followed by zeros.
         */
        remainder = dividend << (WIDTH - 8);

        /*
         * Perform modulo-2 division, a bit at a time.
         */
        for (uint8_t bit = 8; bit > 0; --bit)
        {
            /*
             * Try to divide the current data bit.
             */
            if (remainder & TOPBIT)
            {
                remainder = (remainder << 1) ^ poly;
            }
            else
            {
                remainder = (remainder << 1);
            }
        }

        /*
         * Store the result into the table.
         */
        crcTable[dividend] = remainder;
    }
}

uint8_t crc8_calculate(uint8_t init, uint8_t const message[], int count)
{
    uint8_t data;
    uint8_t remainder = init;

    /*
     * Divide the message by the polynomial, a byte at a time.
     */
    for (int byte = 0; byte < count; ++byte)
    {
        data = message[byte] ^ (remainder >> (WIDTH - 8));
        remainder = crcTable[data] ^ (remainder << 8);
    }

    /*
     * The final remainder is the CRC.
     */
    return (remainder);
}
