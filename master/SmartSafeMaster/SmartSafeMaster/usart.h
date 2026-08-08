#ifndef USART_H
#define USART_H

#include <stdint.h>

void usart_init(uint16_t ubrr);
void usart_tx(uint8_t c);
void usart_print(const char *s);
void usart_print_and_wait(const char *s);
uint8_t usart_rx_available(void);
uint8_t usart_rx(void);

#endif /* USART_H */
