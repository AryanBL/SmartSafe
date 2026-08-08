/*
 * config.h — SmartSafe Slave / Remote Alarm hardware map
 * Target: ATmega32 @ 8 MHz
 *
 * The Slave receives exact event lines from the Master through USART.
 * It owns the remote LCD, green LED, red LED and buzzer.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <avr/io.h>
#include <stdint.h>

#ifndef F_CPU
#define F_CPU 8000000UL
#endif

/* 9600 baud @ 8 MHz, normal asynchronous mode. */
#define USART_BAUD_UBRR       51U

/* ---------------- LCD 16x2, 8-bit mode ----------------
 * Data:    PC0..PC7
 * Control: PA0 = RS, PA2 = EN
 * LCD R/W must be tied directly to GND.
 */
#define LCD_DATA_DDR          DDRC
#define LCD_DATA_PORT         PORTC
#define LCD_CTRL_DDR          DDRA
#define LCD_CTRL_PORT         PORTA
#define LCD_RS                PA0
#define LCD_EN                PA2
#define LCD_CTRL_MASK         ((1U << LCD_RS) | (1U << LCD_EN))

/* ---------------- Remote indicators ---------------- */
#define BUZ_DDR               DDRA
#define BUZ_PORT              PORTA
#define BUZ_PIN               PA3

#define LED_DDR               DDRD
#define LED_PORT              PORTD
#define GREEN_LED             PD6
#define RED_LED               PD7

/* Temporary indications required by the project specification. */
#define REMOTE_ALARM_SEC      5U
#define BREACH_ALARM_SEC   30U
#define SUCCESS_LED_SEC       5U

/* Largest accepted USART line, excluding the terminating '\0'. */
#define RX_LINE_MAX           79U

#endif /* CONFIG_H */
