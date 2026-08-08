#include <avr/io.h>
#include "usart.h"

void usart_init(uint16_t ubrr)
{
    UBRRH = (uint8_t)(ubrr >> 8);
    UBRRL = (uint8_t)ubrr;
    UCSRB = (1 << RXEN) | (1 << TXEN);
    UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0); /* 8N1 */
}

void usart_tx(uint8_t c)
{
    while (!(UCSRA & (1 << UDRE))) {
        /* wait */
    }
    UDR = c;
}

void usart_print(const char *s)
{
    while (*s)
        usart_tx((uint8_t)*s++);
}

void usart_print_and_wait(const char *s)
{
    uint8_t writable_status_bits = UCSRA & ((1 << U2X) | (1 << MPCM));

    /* TXC is write-one-to-clear. Preserve the writable mode bits. */
    UCSRA = writable_status_bits | (1 << TXC);
    usart_print(s);

    /* Wait until the final stop bit has left TXD, not merely UDR. */
    while (!(UCSRA & (1 << TXC))) {
        /* wait */
    }
}

uint8_t usart_rx_available(void)
{
    return (UCSRA & (1 << RXC)) ? 1 : 0;
}

uint8_t usart_rx(void)
{
    while (!(UCSRA & (1 << RXC))) {
        /* wait */
    }
    return UDR;
}
