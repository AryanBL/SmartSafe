#ifndef SPI_EEPROM_H
#define SPI_EEPROM_H

#include <stdint.h>

void spi_eeprom_init(void);
void ext_eeprom_write_byte(uint16_t addr, uint8_t data);
uint8_t ext_eeprom_read_byte(uint16_t addr);

#endif /* SPI_EEPROM_H */
