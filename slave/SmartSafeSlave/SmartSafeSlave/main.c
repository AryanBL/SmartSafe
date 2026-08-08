/*
 * main.c — SmartSafe Slave / Remote Alarm Unit
 * Target: ATmega32 @ 8 MHz
 *
 * Protocol accepted from the final Master:
 *   LOGIN_OK, FAILURE, BREACH, Tamper, POWER_FAIL,
 *   POWER RESTORED, WDT_RESET
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "LCD.h"
#include "usart.h"
#include "timer.h"

static uint8_t temporary_alarm_active = 0;
static uint8_t power_alarm_active = 0;
static uint8_t green_active = 0;
static uint8_t log_dump_active = 0;

static void disable_jtag_for_lcd_portc(void)
{
#ifdef JTD
    MCUCSR |= (1U << JTD);
    MCUCSR |= (1U << JTD);
#endif
}

static void lcd_write_line(uint8_t row, const char *text)
{
    uint8_t i;

    LCD_set_cursor(row, 0);

    for (i = 0; i < 16U; i++) {
        if (*text != '\0') {
            LCD_write((uint8_t)*text);
            text++;
        } else {
            LCD_write((uint8_t)' ');
        }
    }
}

static void lcd_show_two_lines(const char *line1, const char *line2)
{
    LCD_clear();
    lcd_write_line(0, line1);
    lcd_write_line(1, line2);
}

static void outputs_off(void)
{
    LED_PORT &= ~((1U << GREEN_LED) | (1U << RED_LED));
    BUZ_PORT &= ~(1U << BUZ_PIN);

    alarm_counter = 0;
    green_counter = 0;
    temporary_alarm_active = 0;
    green_active = 0;
}

static void temporary_alarm_clear(void)
{
    temporary_alarm_active = 0;
    alarm_counter = 0;

    LED_PORT &= ~(1U << RED_LED);
    BUZ_PORT &= ~(1U << BUZ_PIN);
}

static void success_start(uint8_t seconds)
{
    /* A persistent power alarm may only be cleared by POWER RESTORED. */
    if (power_alarm_active)
        return;

    LED_PORT &= ~(1U << RED_LED);
    BUZ_PORT &= ~(1U << BUZ_PIN);
    LED_PORT |= (1U << GREEN_LED);

    temporary_alarm_active = 0;
    alarm_counter = 0;
    green_active = 1;
    green_counter = seconds;
}

static void temporary_alarm_start(uint8_t seconds)
{
    if (power_alarm_active)
        return;

    LED_PORT &= ~(1U << GREEN_LED);
    LED_PORT |= (1U << RED_LED);
    BUZ_PORT |= (1U << BUZ_PIN);

    green_active = 0;
    green_counter = 0;
    temporary_alarm_active = 1;
    alarm_counter = seconds;
}

static void power_alarm_start(void)
{
    power_alarm_active = 1;
    temporary_alarm_active = 0;
    green_active = 0;
    alarm_counter = 0;
    green_counter = 0;

    LED_PORT &= ~(1U << GREEN_LED);
    LED_PORT |= (1U << RED_LED);
    BUZ_PORT |= (1U << BUZ_PIN);
}

static void power_alarm_clear(void)
{
    power_alarm_active = 0;
    outputs_off();
}

static void service_indicator_timeouts(void)
{
    if (temporary_alarm_active && alarm_counter == 0U) {
        temporary_alarm_active = 0;
        LED_PORT &= ~(1U << RED_LED);
        BUZ_PORT &= ~(1U << BUZ_PIN);
    }

    if (green_active && green_counter == 0U) {
        green_active = 0;
        LED_PORT &= ~(1U << GREEN_LED);
    }
}

static void process_event_message(const char *msg)
{
    /* Exact comparisons prevent log-dump lines from triggering alarms. */
    if (strcmp(msg, "LOGIN_OK") == 0) {
        lcd_show_two_lines("ACCESS GRANTED", "SAFE OPEN");
        success_start(SUCCESS_LED_SEC);
        return;
    }

    if (strcmp(msg, "FAILURE") == 0) {
        lcd_show_two_lines("WRONG PASSWORD", "TRY AGAIN");
        temporary_alarm_start(REMOTE_ALARM_SEC);
        return;
    }
    if (strcmp(msg, "LOCKOUT_END") == 0) {
        temporary_alarm_clear();

        lcd_show_two_lines(
            "REMOTE ALARM",
            "WAITING...");

        return;
    }

    if (strcmp(msg, "BREACH") == 0) {
        lcd_show_two_lines("SECURITY BREACH", "MASTER LOCKED");
        temporary_alarm_start(BREACH_ALARM_SEC);
        return;
    }

    if (strcmp(msg, "Tamper") == 0) {
        lcd_show_two_lines("TAMPER DETECTED", "CHECK SAFE");
        temporary_alarm_start(REMOTE_ALARM_SEC);
        return;
    }

    if (strcmp(msg, "POWER_FAIL") == 0) {
        lcd_show_two_lines("POWER FAILURE", "MASTER OFFLINE");
        power_alarm_start();
        return;
    }

    if (strcmp(msg, "POWER RESTORED") == 0) {
        power_alarm_clear();
        lcd_show_two_lines("POWER RESTORED", "SYSTEM NORMAL");
        return;
    }

    if (strcmp(msg, "WDT_RESET") == 0) {
        lcd_show_two_lines("MASTER RESTARTED", "WATCHDOG RESET");
        return;
    }

    /* Unrecognized lines, including LOG CLEARED, are deliberately ignored. */
}

static void process_received_line(const char *line)
{
    if (strcmp(line, "=== LOG DUMP ===") == 0) {
        log_dump_active = 1;
        return;
    }

    if (log_dump_active) {
        if (strcmp(line, "=== END ===") == 0)
            log_dump_active = 0;

        return;
    }

    /* A stray end marker is also ignored. */
    if (strcmp(line, "=== END ===") == 0)
        return;

    process_event_message(line);
}

static void system_init(void)
{
    disable_jtag_for_lcd_portc();

    LED_DDR |= (1U << GREEN_LED) | (1U << RED_LED);
    LED_PORT &= ~((1U << GREEN_LED) | (1U << RED_LED));

    BUZ_DDR |= (1U << BUZ_PIN);
    BUZ_PORT &= ~(1U << BUZ_PIN);

    usart_init(USART_BAUD_UBRR);
    LCD_init();
    timer2_init();

    sei();

    lcd_show_two_lines("REMOTE ALARM", "WAITING...");
}

int main(void)
{
    char line[RX_LINE_MAX + 1U];
    uint8_t position = 0;
    uint8_t overflow = 0;

    wdt_disable();
    system_init();
    wdt_enable(WDTO_2S);

    memset(line, 0, sizeof(line));

    for (;;) {
        char c;

        wdt_reset();
        service_indicator_timeouts();

        if (!usart_rx_available())
            continue;

        c = (char)usart_rx();

        if (c == '\r' || c == '\n') {
            if (!overflow && position > 0U) {
                line[position] = '\0';
                process_received_line(line);
            }

            position = 0;
            overflow = 0;
            line[0] = '\0';
            continue;
        }

        if (overflow)
            continue;

        if (position < RX_LINE_MAX) {
            line[position++] = c;
        } else {
            /* Discard the whole overlong line until its CR/LF terminator. */
            overflow = 1;
            position = 0;
        }
    }
}
