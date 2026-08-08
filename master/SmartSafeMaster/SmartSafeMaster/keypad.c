#include <avr/io.h>
#include "config.h"
#include "keypad.h"

typedef struct {
    volatile uint8_t *ddr;
    volatile uint8_t *port;
    volatile uint8_t *pin;
    uint8_t bit;
} gpio_t;

static const gpio_t rows[4] = {
    { &DDRA, &PORTA, &PINA, PA4 },
    { &DDRA, &PORTA, &PINA, PA5 },
    { &DDRA, &PORTA, &PINA, PA6 },
    { &DDRA, &PORTA, &PINA, PA7 }
};

static const gpio_t cols[4] = {
    { &DDRD, &PORTD, &PIND, PD3 },
    { &DDRD, &PORTD, &PIND, PD4 },
    { &DDRB, &PORTB, &PINB, PB0 },
    { &DDRB, &PORTB, &PINB, PB1 }
};

static const char keymap[4][4] = {
    { '7', '8', '9', '/' },
    { '4', '5', '6', '*' },
    { '1', '2', '3', '-' },
    { 'C', '0', 'E', '+' }
};

static void gpio_output_high(const gpio_t *g)
{
    *(g->ddr)  |=  (1 << g->bit);
    *(g->port) |=  (1 << g->bit);
}

static void gpio_output_low(const gpio_t *g)
{
    *(g->ddr)  |=  (1 << g->bit);
    *(g->port) &= ~(1 << g->bit);
}

static void gpio_input_pullup(const gpio_t *g)
{
    *(g->ddr)  &= ~(1 << g->bit);
    *(g->port) |=  (1 << g->bit);
}

static uint8_t gpio_is_low(const gpio_t *g)
{
    return ((*(g->pin) & (1 << g->bit)) == 0);
}

static void keypad_configure_pins(void)
{
    for (uint8_t r = 0; r < 4; r++)
        gpio_output_high(&rows[r]);

    for (uint8_t c = 0; c < 4; c++)
        gpio_input_pullup(&cols[c]);
}

char keypad_scan(void)
{
    static char last_reported = 0;
    char pressed = 0;

    keypad_configure_pins();

    for (uint8_t r = 0; r < 4; r++) {
        for (uint8_t rr = 0; rr < 4; rr++)
            gpio_output_high(&rows[rr]);

        gpio_output_low(&rows[r]);
        __asm__ volatile ("nop\nnop\nnop\nnop\n");

        for (uint8_t c = 0; c < 4; c++) {
            if (gpio_is_low(&cols[c])) {
                pressed = keymap[r][c];
                break;
            }
        }

        if (pressed)
            break;
    }

    for (uint8_t r = 0; r < 4; r++)
        gpio_output_high(&rows[r]);

    /* Edge behavior: return a key once, not repeatedly while it is held. */
    if (!pressed) {
        last_reported = 0;
        return 0;
    }

    if (pressed == last_reported)
        return 0;

    last_reported = pressed;
    return pressed;
}
