#include <avr/io.h>
#include "adc.h"

void adc_init(void)
{
    ADMUX = (1 << REFS0); /* AVCC reference, right adjusted */
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1); /* /64 -> 125 kHz */
}

uint16_t adc_read(uint8_t channel)
{
    ADMUX = (ADMUX & 0xE0) | (channel & 0x07); /* keep REFS and ADLAR */
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC)) {
        /* wait */
    }
    return ADC;
}
