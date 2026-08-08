/*
 * LCD.c — 16x2 LCD driver, 8-bit mode
 * Exception allowed by project review: this file uses <util/delay.h>.
 * It only changes the LCD control bits and does not clobber other PORTA pins.
 */

#include "config.h"
#include "LCD.h"
#include <util/delay.h>

static void lcd_set_rs(uint8_t is_data)
{
    if (is_data)
        LCD_CTRL_PORT |= (1 << LCD_RS);
    else
        LCD_CTRL_PORT &= ~(1 << LCD_RS);
}

static void lcd_pulse_enable(void)
{
    LCD_CTRL_PORT |= (1 << LCD_EN);
    _delay_us(2);
    LCD_CTRL_PORT &= ~(1 << LCD_EN);
    _delay_us(50);
}

void LCD_cmd(uint8_t cmd)
{
    lcd_set_rs(0);
    LCD_DATA_PORT = cmd;
    lcd_pulse_enable();

    if (cmd == 0x01 || cmd == 0x02)
        _delay_ms(2);
}

void LCD_write(uint8_t data)
{
    lcd_set_rs(1);
    LCD_DATA_PORT = data;
    lcd_pulse_enable();
}

void LCD_write_string(const char *s)
{
    while (*s)
        LCD_write((uint8_t)*s++);
}

void LCD_init(void)
{
    LCD_DATA_DDR = 0xFF;
    LCD_CTRL_DDR |= LCD_CTRL_MASK;
    LCD_CTRL_PORT &= ~LCD_CTRL_MASK;

    _delay_ms(20);
    LCD_cmd(0x38);      /* 8-bit, 2-line, 5x7 */
    _delay_ms(5);
    LCD_cmd(0x0C);      /* display on, cursor off */
    LCD_cmd(0x06);      /* entry mode: increment */
    LCD_clear();
}

void LCD_set_cursor(uint8_t row, uint8_t col)
{
    uint8_t base = (row == 0) ? 0x80 : 0xC0;
    LCD_cmd((uint8_t)(base + col));
}

void LCD_clear(void)
{
    LCD_cmd(0x01);
}

void LCD_off(void)
{
    LCD_cmd(0x08);
}

void LCD_on(void)
{
    LCD_cmd(0x0C);
}
