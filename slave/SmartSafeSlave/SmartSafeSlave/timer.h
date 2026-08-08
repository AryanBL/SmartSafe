#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* One-second countdowns updated by Timer2. */
extern volatile uint8_t alarm_counter;
extern volatile uint8_t green_counter;

void timer2_init(void);

/* Timer0 polling delays; no prepared delay library is used. */
void delay_ms(uint16_t ms);
void delay_us(uint16_t us);

#endif /* TIMER_H */
