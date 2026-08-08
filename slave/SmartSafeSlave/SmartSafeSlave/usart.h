#ifndef USART_H
#define USART_H

#include <stdint.h>

void usart_init(uint16_t ubrr);
uint8_t usart_rx_available(void);
uint8_t usart_rx(void);

#endif /* USART_H */
