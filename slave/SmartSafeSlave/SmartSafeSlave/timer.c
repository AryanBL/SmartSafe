#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer.h"

volatile uint8_t alarm_counter = 0;
volatile uint8_t green_counter = 0;

static volatile uint8_t sub10ms = 0;
static volatile uint8_t sub100ms = 0;

ISR(TIMER2_COMP_vect)
{
    /* OCR2=77 with /1024 gives approximately 9.984 ms at 8 MHz. */
    if (++sub10ms < 10U)
        return;

    sub10ms = 0;

    if (++sub100ms < 10U)
        return;

    sub100ms = 0;

    if (alarm_counter > 0U)
        alarm_counter--;

    if (green_counter > 0U)
        green_counter--;
}

void timer2_init(void)
{
    uint8_t sreg = SREG;
    cli();

    TCCR2 = 0;
    TCNT2 = 0;
    OCR2 = 77;
    TIFR = (1U << OCF2); /* OCF2 is write-one-to-clear. */

    sub10ms = 0;
    sub100ms = 0;
    alarm_counter = 0;
    green_counter = 0;

    TIMSK |= (1U << OCIE2);
    TCCR2 = (1U << WGM21) |
            (1U << CS22)  |
            (1U << CS21)  |
            (1U << CS20);

    SREG = sreg;
}

void delay_ms(uint16_t ms)
{
    uint8_t saved_tccr0 = TCCR0;
    uint8_t saved_timsk = TIMSK;
    uint8_t saved_ocr0  = OCR0;
    uint8_t saved_tcnt0 = TCNT0;

    TCCR0 = 0;
    TIMSK &= ~(1U << OCIE0);

    /*
     * 8 MHz / 64 = 125 kHz
     * One count = 8 us
     * 125 counts = 1 ms
     */
    OCR0 = 124U;
    TCNT0 = 0;
    TIFR = (1U << OCF0);

    /* CTC mode, /64. */
    TCCR0 = (1U << WGM01) |
            (1U << CS01) |
            (1U << CS00);

    while (ms-- > 0U) {
        TCNT0 = 0;
        TIFR = (1U << OCF0);

        while (!(TIFR & (1U << OCF0))) {
            /* wait for approximately 1 ms */
        }
    }

    TCCR0 = 0;
    TIFR = (1U << OCF0);

    OCR0 = saved_ocr0;
    TCNT0 = saved_tcnt0;
    TIMSK = saved_timsk;
    TCCR0 = saved_tccr0;
}

void delay_us(uint16_t us)
{
    uint8_t saved_tccr0 = TCCR0;
    uint8_t saved_timsk = TIMSK;
    uint8_t saved_ocr0  = OCR0;
    uint8_t saved_tcnt0 = TCNT0;

    /*
     * Stop Timer0 before configuring it.
     * The delay uses flag polling, not a Timer0 ISR.
     */
    TCCR0 = 0;
    TIMSK &= ~(1U << OCIE0);

    /*
     * At F_CPU = 8 MHz with no prescaler:
     *
     * Timer clock = 8 MHz
     * One timer count = 0.125 us
     *
     * Eight counts produce 1 us.
     * Timer counts from 0 through 7, so OCR0 = 7.
     */
    OCR0 = 7U;
    TCNT0 = 0;
    TIFR = (1U << OCF0);

    /* CTC mode, no prescaler. */
    TCCR0 = (1U << WGM01) | (1U << CS00);

    while (us-- > 0U) {
        TCNT0 = 0;
        TIFR = (1U << OCF0);

        while (!(TIFR & (1U << OCF0))) {
            /* wait for approximately 1 us */
        }
    }

    /*
     * Stop Timer0 and restore its previous configuration.
     */
    TCCR0 = 0;
    TIFR = (1U << OCF0);

    OCR0 = saved_ocr0;
    TCNT0 = saved_tcnt0;
    TIMSK = saved_timsk;
    TCCR0 = saved_tccr0;
}