#include <avr/io.h>
#include <avr/interrupt.h>
#include "config.h"
#include "password.h"

static uint8_t ee_read(uint16_t addr)
{
    while (EECR & (1 << EEWE)) {
        /* wait */
    }
    EEAR = addr;
    EECR |= (1 << EERE);
    return EEDR;
}

static void ee_write(uint16_t addr, uint8_t val)
{
    while (EECR & (1 << EEWE)) {
        /* wait */
    }

    EEAR = addr;
    EEDR = val;

    uint8_t sreg = SREG;
    cli();
    EECR |= (1 << EEMWE);
    EECR |= (1 << EEWE);
    SREG = sreg;
}

void password_init(void)
{
    if (ee_read(INT_EE_SIGNATURE_ADDR) != INT_EE_SIGNATURE_VAL) {
        ee_write(INT_EE_PASSWORD_ADDR + 0, '1');
        ee_write(INT_EE_PASSWORD_ADDR + 1, '2');
        ee_write(INT_EE_PASSWORD_ADDR + 2, '3');
        ee_write(INT_EE_PASSWORD_ADDR + 3, '4');
        ee_write(INT_EE_FAIL_COUNT_ADDR, 0);
        ee_write(INT_EE_SIGNATURE_ADDR, INT_EE_SIGNATURE_VAL);
    }

    if (ee_read(INT_EE_FAIL_COUNT_ADDR) > MAX_FAIL_ATTEMPTS)
        ee_write(INT_EE_FAIL_COUNT_ADDR, 0);
}

uint8_t password_check(const char *entered4)
{
    for (uint8_t i = 0; i < 4; i++) {
        if ((uint8_t)entered4[i] != ee_read(INT_EE_PASSWORD_ADDR + i))
            return 0;
    }
    return 1;
}

void password_change(const char *new_pass4)
{
    for (uint8_t i = 0; i < 4; i++)
        ee_write(INT_EE_PASSWORD_ADDR + i, (uint8_t)new_pass4[i]);
    password_reset_fail();
}

uint8_t password_get_fail_count(void)
{
    return ee_read(INT_EE_FAIL_COUNT_ADDR);
}

uint8_t password_increment_fail(void)
{
    uint8_t count = ee_read(INT_EE_FAIL_COUNT_ADDR);
    if (count < 255)
        count++;
    ee_write(INT_EE_FAIL_COUNT_ADDR, count);
    return count;
}

void password_reset_fail(void)
{
    ee_write(INT_EE_FAIL_COUNT_ADDR, 0);
}
