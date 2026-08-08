#ifndef LCD_H
#define LCD_H

#include <stdint.h>

void LCD_init(void);
void LCD_cmd(uint8_t cmd);
void LCD_write(uint8_t data);
void LCD_write_string(const char *s);
void LCD_set_cursor(uint8_t row, uint8_t col);
void LCD_clear(void);
void LCD_off(void);
void LCD_on(void);

#endif /* LCD_H */
