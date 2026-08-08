#include <avr/io.h>
#include "config.h"
#include "servo.h"

void servo_init(void)
{
    SERVO_DDR |= (1 << SERVO_PIN);

    /* Timer1 Fast PWM mode 14, TOP=ICR1, non-inverting OC1A, prescaler /8.
     * 8 MHz / 8 = 1 MHz -> 1 us per timer count.
     * ICR1=19999 -> 20 ms period -> 50 Hz servo frame.
     */
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
    ICR1 = 19999;
    servo_lock();
}

void servo_set(uint8_t angle)
{
    if (angle > 180)
        angle = 180;

    OCR1A = (uint16_t)(1000UL + ((uint32_t)angle * 1000UL) / 180UL);
}

void servo_lock(void)
{
    servo_set(0);
}

void servo_unlock(void)
{
    servo_set(90);
}
