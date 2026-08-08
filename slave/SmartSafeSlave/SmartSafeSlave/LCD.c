/* LCD.c — HD44780-compatible 16x2 LCD, 8-bit mode. */

#include "config.h"
#include "LCD.h"
#include "timer.h"

static void lcd_set_rs(uint8_t is_data)
{
    if (is_data)
        LCD_CTRL_PORT |= (1U << LCD_RS);
    else
        LCD_CTRL_PORT &= ~(1U << LCD_RS);
}

static void lcd_pulse_enable(void)
{
    /*
     * Give RS and PORTC data time to stabilize before E rises.
     */
    delay_us(2);

    LCD_CTRL_PORT |= (1U << LCD_EN);
    delay_us(2);

    LCD_CTRL_PORT &= ~(1U << LCD_EN);
}

void LCD_cmd(uint8_t cmd)
{
    lcd_set_rs(0);
    LCD_DATA_PORT = cmd;
    lcd_pulse_enable();

    /*
     * Clear Display and Return Home are slow commands.
     * Use 3 ms for reliable Proteus operation.
     */
    if (cmd == 0x01U || cmd == 0x02U) {
        delay_ms(3);
    } else {
        /*
         * Normal HD44780 commands generally need about 37 us.
         * Use 100 us for safe simulation margin.
         */
        delay_us(100);
    }
}

void LCD_write(uint8_t data)
{
    lcd_set_rs(1);
    LCD_DATA_PORT = data;
    lcd_pulse_enable();

    /*
     * Allow the LCD to finish writing the character before
     * placing the next character on the bus.
     */
    delay_us(100);
}

void LCD_write_string(const char *s)
{
    while (*s != '\0')
        LCD_write((uint8_t)*s++);
}

void LCD_init(void)
{
    LCD_DATA_DDR = 0xFFU;
    LCD_CTRL_DDR |= LCD_CTRL_MASK;
    LCD_CTRL_PORT &= ~LCD_CTRL_MASK;

    delay_ms(20);
    LCD_cmd(0x38); /* 8-bit, 2-line, 5x8 font */
    delay_ms(5);
    LCD_cmd(0x0C); /* display on, cursor off */
    LCD_cmd(0x06); /* increment cursor */
    LCD_clear();
}

void LCD_set_cursor(uint8_t row, uint8_t col)
{
    uint8_t base = (row == 0U) ? 0x80U : 0xC0U;
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
