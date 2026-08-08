#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

extern volatile uint8_t  timer_tick_100ms;
extern volatile uint32_t system_time_s;
extern volatile uint8_t  activity_timer;

extern volatile uint8_t lockout_active;
extern volatile uint8_t servo_open;

extern volatile uint8_t lockout_counter;
extern volatile uint8_t servo_counter;

void timer2_init(void);
void delay_ms(uint16_t ms);
void delay_us(uint16_t us);

/* Atomic 32-bit timestamp read for the 8-bit AVR core. */
uint32_t timer_get_seconds(void);

#endif /* TIMER_H */
