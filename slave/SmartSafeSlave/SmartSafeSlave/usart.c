#include <avr/io.h>
#include "usart.h"

void usart_init(uint16_t ubrr)
{
    UBRRH = (uint8_t)(ubrr >> 8);
    UBRRL = (uint8_t)ubrr;

    /* The final Slave is a receive-only endpoint. */
    UCSRB = (1U << RXEN);

    /* URSEL selects UCSRC on ATmega32; UCSZ1:0=11 gives 8N1. */
    UCSRC = (1U << URSEL) | (1U << UCSZ1) | (1U << UCSZ0);
}

uint8_t usart_rx_available(void)
{
    return (UCSRA & (1U << RXC)) ? 1U : 0U;
}

uint8_t usart_rx(void)
{
    while (!(UCSRA & (1U << RXC))) {
        /* wait */
    }

    return UDR;
}
