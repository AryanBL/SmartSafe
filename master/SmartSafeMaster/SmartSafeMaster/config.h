/*
 * config.h — SmartSafe Master hardware map and constants
 * Target: ATmega32 @ 8 MHz
 *
 * Design choices fixed in this reimplementation:
 *  - Real ATmega32 hardware SPI is used on PB4..PB7.
 *  - Keypad was moved away from PORTB high nibble to avoid SPI conflict.
 *  - LCD R/W is tied to GND in hardware; the software uses only RS and EN.
 *  - Tamper potentiometer uses PA1/ADC1, no conflict with LCD R/W.
 *  - Power-fail detection uses the analog comparator with internal bandgap.
 *    On ATmega32, ACBG connects bandgap to the positive comparator input;
 *    therefore the voltage divider output must be connected to AIN1/PB3.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <avr/io.h>
#include <stdint.h>

#ifndef F_CPU
#define F_CPU 8000000UL
#endif

/* USART: 9600 baud @ 8 MHz, normal speed: UBRR = F_CPU/(16*BAUD)-1 = 51 */
#define USART_BAUD_UBRR     51

/* ---------------- LCD 16x2, 8-bit mode ----------------
 * Data:    PC0..PC7
 * Control: PA0 = RS, PA2 = EN
 * LCD R/W pin must be connected to GND.
 */
#define LCD_DATA_DDR        DDRC
#define LCD_DATA_PORT       PORTC
#define LCD_CTRL_DDR        DDRA
#define LCD_CTRL_PORT       PORTA
#define LCD_RS              PA0
#define LCD_EN              PA2
#define LCD_CTRL_MASK       ((1 << LCD_RS) | (1 << LCD_EN))

/* ---------------- Servo lock ----------------
 * OC1A = PD5 on ATmega32.
 */
#define SERVO_DDR           DDRD
#define SERVO_PIN           PD5

/* ---------------- Wake button ----------------
 * INT0 = PD2. Button should pull pin low when pressed.
 */
#define BTN_DDR             DDRD
#define BTN_PORT            PORTD
#define BTN_PIN             PD2

/* ---------------- Keypad 4x4 ----------------
 * Mixed-port keypad mapping to avoid SPI, USART, servo, ADC and comparator pins.
 * Rows are outputs, columns are inputs with internal pull-ups.
 * Rows: PA4 PA5 PA6 PA7
 * Cols: PD3 PD4 PB0 PB1
 */
#define KP_ROWS             4
#define KP_COLS             4

/* ---------------- ADC ---------------- */
#define ADC_TAMPER_CH       1       /* PA1/ADC1: tamper potentiometer */

/* ---------------- Analog comparator power monitor ----------------
 * With ACBG=1, internal bandgap is comparator positive input.
 * Connect the simulated/monitored divider output to AIN1/PB3.
 * Comparator output ACO=1 means bandgap > divider => supply below threshold.
 *
 * After a confirmed failure, the Master sends/logs POWER_FAIL, disables the
 * other peripherals, and sleeps in Idle with only the comparator interrupt
 * active. A restored divider voltage wakes and reinitializes normal operation.
 */
#define POWER_SENSE_DDR     DDRB
#define POWER_SENSE_PORT    PORTB
#define POWER_SENSE_PIN     PB3     /* AIN1 */

/*
 * Software comparator stability filter. Timer2 ticks approximately every 10 ms.
 * A new comparator state must remain unchanged for about 100 ms before the
 * program reports POWER_FAIL or POWER RESTORED. Increase this value if the
 * simulated or physical divider is especially noisy.
 */
#define POWER_CONFIRM_TICKS_10MS  10U

/* ---------------- Hardware SPI: 25LC040 EEPROM ----------------
 * ATmega32 real SPI pins:
 * SS   PB4 -> /CS
 * MOSI PB5 -> SI
 * MISO PB6 -> SO
 * SCK  PB7 -> SCK
 */
#define EE_SPI_DDR          DDRB
#define EE_SPI_PORT         PORTB
#define EE_SPI_PIN          PINB
#define EE_SPI_SS           PB4
#define EE_SPI_MOSI         PB5
#define EE_SPI_MISO         PB6
#define EE_SPI_SCK          PB7

#define EE25_WREN           0x06
#define EE25_WRITE          0x02
#define EE25_READ           0x03
#define EE25_RDSR           0x05

/* ---------------- Internal EEPROM layout ---------------- */
#define INT_EE_SIGNATURE_ADDR   0x00
#define INT_EE_SIGNATURE_VAL    0xA5
#define INT_EE_PASSWORD_ADDR    0x01    /* 4 bytes: password */
#define INT_EE_FAIL_COUNT_ADDR  0x05    /* 1 byte: failed attempts */

/* ---------------- External EEPROM log layout ----------------
 * 25LC040 = 512 bytes. We reserve 8 bytes for metadata and use the rest for logs.
 * Header:
 *   0: magic 0x53 ('S')
 *   1: magic 0x46 ('F')
 *   2: next write index, 0..62
 *   3: valid record count, 0..63
 *   4..7: reserved
 * Record: 8 bytes
 *   0: event code
 *   1..4: timestamp, big-endian seconds
 *   5..6: data, big-endian 16-bit optional value
 *   7: simple checksum = XOR of bytes 0..6
 */
#define LOG_HEADER_ADDR     0x000
#define LOG_BASE_ADDR       0x008
#define LOG_RECORD_SIZE     8
#define LOG_MAX_RECORDS     63      /* 63*8 = 504 bytes + 8-byte header = 512 */
#define LOG_MAGIC0          0x53
#define LOG_MAGIC1          0x46

/* Event codes */
#define EVT_LOGIN_OK        0x01
#define EVT_LOGIN_FAIL      0x02
#define EVT_LOCKOUT         0x03
#define EVT_TAMPER          0x04
#define EVT_POWER_FAIL      0x05
#define EVT_POWER_OK        0x06
#define EVT_WDT_RESET       0x07
#define EVT_SLEEP           0x08
#define EVT_WAKE            0x09

/* Behavior constants */
#define MAX_FAIL_ATTEMPTS   3
#define LOCKOUT_SECONDS     30
#define SLEEP_TIMEOUT_SEC   10
#define SERVO_OPEN_SEC      5
/*
 * These hold periods temporarily block Master keypad input. They do not place
 * event text on the Master LCD; the LCD remains the password-entry interface.
 */
#define TAMPER_UI_HOLD_SEC  5
#define TAMPER_THRESHOLD    50
#define FAIL_UI_HOLD_SEC    1

/* Messages sent to the independently controlled Slave unit.
 * The Master has no local LEDs or buzzer; the Slave interprets these messages
 * and controls its own indicators.
 */
#define MSG_LOGIN_OK        "LOGIN_OK\r\n"
#define MSG_FAILURE         "FAILURE\r\n"
#define MSG_BREACH          "BREACH\r\n"
#define MSG_LOCKOUT_END  "LOCKOUT_END\r\n"
#define MSG_TAMPER          "Tamper\r\n"
#define MSG_POWER_FAIL      "POWER_FAIL\r\n"
#define MSG_POWER_OK        "POWER RESTORED\r\n"
#define MSG_WDT_RESET       "WDT_RESET\r\n"

#endif /* CONFIG_H */
