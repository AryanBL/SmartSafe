/*
 * main.c — SmartSafe Master Unit
 * Target: ATmega32 @ 8 MHz
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <stdint.h>
#include <string.h>

#include "config.h"
#include "LCD.h"
#include "timer.h"
#include "usart.h"
#include "keypad.h"
#include "servo.h"
#include "adc.h"
#include "tamper.h"
#include "spi_eeprom.h"
#include "logger.h"
#include "password.h"
#include "power.h"

/*
 * Indicator ownership: the Master has no local LEDs or buzzer.  It detects
 * events and reports them over USART; the Slave owns all visual/audible alarms.
 */

ISR(INT0_vect)
{
    /*
     * INT0 is used only to wake the MCU from inactivity Power-down.
     * Low-level mode remains active while the button is held, so disable INT0
     * immediately. It will be enabled again only before the next sleep.
     */
    GICR &= ~(1 << INT0);
}

static uint8_t time_reached(uint32_t target)
{
    uint32_t now = timer_get_seconds();
    return ((int32_t)(now - target) >= 0);
}

static void disable_jtag_for_lcd_portc(void)
{
#ifdef JTD
    uint8_t saved_sreg = SREG;
    uint8_t value;

    cli();

    value = MCUCSR | (1U << JTD);

    /*
     * Precompute the value, then perform two consecutive writes.
     * This satisfies the timed JTD sequence.
     */
    MCUCSR = value;
    MCUCSR = value;

    SREG = saved_sreg;
#endif
}
static void safe_lock(void)
{
    servo_lock();
    servo_open = 0;
    servo_counter = 0;
}

static void safe_unlock(void)
{
    servo_unlock();
    servo_open = 1;
    servo_counter = SERVO_OPEN_SEC;
}

static void lcd_show_prompt(void)
{
    /*
     * The Master LCD is only a password-entry interface. Event/status text is
     * owned by the Slave LCD. While awake, the Master shows only this prompt
     * and one '*' for every entered password digit on the second row.
     */
    LCD_clear();
    LCD_set_cursor(0, 0);
    LCD_write_string("Enter Password:");
    LCD_set_cursor(1, 0);
}

static void restore_runtime_after_power(void)
{
    /* Restore every peripheral that was deliberately disabled in standby. */
    usart_init(USART_BAUD_UBRR);
    LCD_init();
    servo_init();
    adc_init();
    tamper_init();
    spi_eeprom_init();
    timer2_init();

    servo_open = 0;
    servo_counter = 0;
    activity_timer = 0;

    wdt_enable(WDTO_2S);
}

static void enter_power_fail_standby(void)
{
    /*
     * The POWER_FAIL message and log record have already completed before this
     * function is entered. Shut down the remaining peripherals and keep only
     * the analog comparator interrupt active to detect restoration.
     */
    LCD_off();

    servo_open = 0;
    servo_counter = 0;

    wdt_disable();

    /* Disable timer interrupts and stop all three timers. */
    TIMSK = 0;
    TCCR0 = 0;
    TCCR1A = 0;
    TCCR1B = 0;
    TCCR2 = 0;
    PORTD &= ~(1 << SERVO_PIN);

    /* Disable INT0, ADC, SPI and USART after communication/logging is done. */
    GICR &= ~(1 << INT0);
    ADCSRA &= ~(1 << ADEN);
    SPCR &= ~(1 << SPE);
    UCSRB = 0;

    power_monitor_prepare_restore_wait();

    /*
     * Idle is used here because the analog comparator can interrupt and wake
     * the CPU from Idle. All other active interrupt sources were disabled.
     */
    set_sleep_mode(SLEEP_MODE_IDLE);

    for (;;) {
        if (power_monitor_accept_restore_if_present())
            break;

        /*
         * Atomic sleep entry prevents a restoration transition from occurring
         * between the state check and the SLEEP instruction. If ACO toggles
         * while interrupts are disabled, the pending comparator interrupt
         * wakes the CPU immediately after sleep is entered.
         */
        cli();
        if (!power_monitor_raw_failed()) {
            sei();
            continue;
        }

        sleep_enable();
        sei();
        sleep_cpu();
        sleep_disable();
    }

    restore_runtime_after_power();
}

/*
 * Returns 1 when a power-failure standby cycle occurred, allowing main() to
 * clear partial password input and redraw the user interface after restoration.
 */
static uint8_t process_power_event(void)
{
    uint8_t pending;
    uint8_t fail;
    uint8_t sreg = SREG;

    cli();
    pending = power_event_pending;
    fail = power_fail_state;
    power_event_pending = 0;
    SREG = sreg;

    if (!pending)
        return 0;

    if (fail) {
        uint32_t now = timer_get_seconds();

        /* Ensure the complete final stop bit reaches the independently powered Slave. */
        usart_print_and_wait(MSG_POWER_FAIL);
        log_event(EVT_POWER_FAIL, now);

        enter_power_fail_standby();
        return 1;
    }

    usart_print(MSG_POWER_OK);
    log_event(EVT_POWER_OK, timer_get_seconds());

    return 0;
}

static void enter_sleep_mode(void)
{
    log_event(EVT_SLEEP, timer_get_seconds());
    LCD_off();

    /* In inactivity Power-down, INT0 must be the only wake source. */
    power_monitor_suspend_for_sleep();

    MCUCR &= ~((1 << ISC01) | (1 << ISC00)); /* INT0 low-level mode */
    GIFR = (1 << INTF0);                     /* clear stale INT0 flag */
    GICR |= (1 << INT0);

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();

    wdt_disable();
    sei();
    sleep_cpu();

    sleep_disable();
    GICR &= ~(1 << INT0);
    GIFR = (1 << INTF0);

    /* Detect any monitored-voltage change that occurred while asleep. */
    power_monitor_resume_after_sleep();

    wdt_enable(WDTO_2S);
    activity_timer = 0;

    LCD_on();
    log_event(EVT_WAKE, timer_get_seconds());
}
/*
static void system_init(void)
{
    disable_jtag_for_lcd_portc();

     INT0 is enabled only immediately before inactivity Power-down. 
    BTN_DDR &= ~(1 << BTN_PIN);
    BTN_PORT |= (1 << BTN_PIN);
    MCUCR &= ~((1 << ISC01) | (1 << ISC00));  low-level wake mode 
    GIFR = (1 << INTF0);
    GICR &= ~(1 << INT0);

    usart_init(USART_BAUD_UBRR);
    LCD_init();
    servo_init();
    adc_init();
    spi_eeprom_init();
    logger_init();
    password_init();
    tamper_init();
    timer2_init();
    power_monitor_init();

    sei();

     The normal password prompt is drawn by main() on its first iteration. 
    LCD_clear();
}*/
	
	
static void system_init(void)
{
	disable_jtag_for_lcd_portc();

	BTN_DDR &= ~(1U << BTN_PIN);
	BTN_PORT |= (1U << BTN_PIN);
	MCUCR &= ~((1U << ISC01) | (1U << ISC00));
	GIFR = (1U << INTF0);
	GICR &= ~(1U << INT0);

	usart_init(USART_BAUD_UBRR);
	usart_print("U");             /* USART completed */

	LCD_init();
	usart_print("L");             /* LCD completed */

	servo_init();
	usart_print("S");             /* Servo completed */

	adc_init();
	usart_print("A");             /* ADC setup completed */

	spi_eeprom_init();
	usart_print("I");             /* SPI setup completed */

	logger_init();
	usart_print("G");             /* External logger completed */

	password_init();
	usart_print("P");             /* Internal EEPROM completed */

	tamper_init();
	usart_print("T");             /* Initial ADC conversion completed */

	timer2_init();
	usart_print("2");             /* Timer2 completed */

	power_monitor_init();
	usart_print("C");             /* Comparator completed */

	sei();

	LCD_clear();
	usart_print("E\r\n");         /* system_init reached its end */

}
int main(void)
{
    uint8_t was_wdt_reset = (MCUCSR & (1 << WDRF)) ? 1U : 0U;
    MCUCSR &= ~(1 << WDRF);
    wdt_disable();

    system_init();
	DDRD |= (1 << PD7);
	PORTD |= (1 << PD7);
    if (was_wdt_reset) {
        usart_print(MSG_WDT_RESET);
        log_event(EVT_WDT_RESET, timer_get_seconds());
    }

    wdt_enable(WDTO_2S);

    char input[5] = {0};
    uint8_t input_pos = 0;
    uint8_t redraw = 1;
    uint32_t ui_hold_until = 0;
    LCD_clear();
    LCD_set_cursor(0, 0);
    LCD_write_string("MASTER LCD OK");

    LCD_set_cursor(1, 0);
    LCD_write_string("TEST");	

    while (1) {
        wdt_reset();

        power_monitor_process();

        if (process_power_event()) {
            input_pos = 0;
            memset(input, 0, sizeof(input));
            ui_hold_until = 0;
            redraw = 1;
            continue;
        }

        if (timer_tick_100ms) {
            uint32_t now;
            timer_tick_100ms = 0;

            if (tamper_check()) {
                now = timer_get_seconds();
                usart_print(MSG_TAMPER);
                log_event(EVT_TAMPER, now);

                /*
                 * Temporarily block keypad input without changing the Master
                 * LCD. Restart inactivity timing when the hold begins.
                 */
                ui_hold_until = now + TAMPER_UI_HOLD_SEC;
                activity_timer = 0;
            }
        }

        if (lockout_active && lockout_counter == 0U) {
            lockout_active = 0;
            password_reset_fail();

            input_pos = 0;
            memset(input, 0, sizeof(input));

            activity_timer = 0;
            redraw = 1;

            usart_print(MSG_LOCKOUT_END);
        }

        if (servo_open && servo_counter == 0) {
            safe_lock();
            redraw = 1;
        }

        if (ui_hold_until && time_reached(ui_hold_until)) {
            ui_hold_until = 0;

            /*
             * A hold may start while a partial password is on screen, such as
             * after tamper detection. Clear both the visible stars and their
             * matching buffer, then give the user a fresh active period.
             */
            input_pos = 0;
            memset(input, 0, sizeof(input));
            activity_timer = 0;
            redraw = 1;
        }

        if (usart_rx_available()) {
            uint8_t c = usart_rx();
            if (c == 'D' || c == 'd')
                log_dump();
            else if (c == 'X' || c == 'x') {
                log_clear();
                usart_print("LOG CLEARED\r\n");
            }
        }

        if (activity_timer >= SLEEP_TIMEOUT_SEC &&
            !lockout_active && !servo_open && !ui_hold_until) {
            enter_sleep_mode();
            input_pos = 0;
            memset(input, 0, sizeof(input));
            redraw = 1;
        }

        if (redraw && !ui_hold_until) {
            lcd_show_prompt();
            redraw = 0;
        }

        if (lockout_active || servo_open || ui_hold_until)
            continue;

        char key = keypad_scan();
        if (!key)
            continue;

        activity_timer = 0;

        if (key == 'C') {
            input_pos = 0;
            memset(input, 0, sizeof(input));
            redraw = 1;
            continue;
        }

        if (key < '0' || key > '9')
            continue;

        if (input_pos < 4) {
            LCD_write('*');
            input[input_pos++] = key;
        }

        if (input_pos == 4) {
            uint32_t now = timer_get_seconds();
            input[4] = '\0';

            if (password_check(input)) {
                password_reset_fail();
                log_event(EVT_LOGIN_OK, now);
                usart_print(MSG_LOGIN_OK);

                safe_unlock();
                ui_hold_until = now + SERVO_OPEN_SEC;
                activity_timer = 0;
            } else {
                uint8_t fails = password_increment_fail();
                log_event_data(EVT_LOGIN_FAIL, now, fails);

                if (fails >= MAX_FAIL_ATTEMPTS) {
                    lockout_active = 1;
                    lockout_counter = LOCKOUT_SECONDS;
                    log_event_data(EVT_LOCKOUT, now, fails);
                    usart_print(MSG_BREACH);
                    activity_timer = 0;
                    redraw = 1;
                } else {
                    usart_print(MSG_FAILURE);
                    ui_hold_until = now + FAIL_UI_HOLD_SEC;
                    activity_timer = 0;
                }
            }

            input_pos = 0;
            memset(input, 0, sizeof(input));
        }
    }
}
