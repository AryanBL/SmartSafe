/* spi_eeprom.c — Hardware SPI driver for 25LC040 on real ATmega32 pins */

#include <avr/io.h>
#include "config.h"
#include "spi_eeprom.h"

static void ee_cs_low(void)
{
    EE_SPI_PORT &= ~(1 << EE_SPI_SS);
}

static void ee_cs_high(void)
{
    EE_SPI_PORT |= (1 << EE_SPI_SS);
}

static uint8_t spi_transfer(uint8_t data)
{
    SPDR = data;
    while (!(SPSR & (1 << SPIF))) {
        /* wait */
    }
    return SPDR;
}

void spi_eeprom_init(void)
{
    EE_SPI_DDR |= (1 << EE_SPI_SS) | (1 << EE_SPI_MOSI) | (1 << EE_SPI_SCK);
    EE_SPI_DDR &= ~(1 << EE_SPI_MISO);
    ee_cs_high();

    /* Enable SPI, Master, mode 0, clock F_CPU/16 = 500 kHz @ 8 MHz. */
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);
    SPSR = 0x00;
}

static void ext_eeprom_write_enable(void)
{
    ee_cs_low();
    spi_transfer(EE25_WREN);
    ee_cs_high();
}

static void ext_eeprom_wait_ready(void)
{
    uint8_t sr;
    do {
        ee_cs_low();
        spi_transfer(EE25_RDSR);
        sr = spi_transfer(0xFF);
        ee_cs_high();
    } while (sr & 0x01); /* WIP bit */
}

void ext_eeprom_write_byte(uint16_t addr, uint8_t data)
{
    addr &= 0x01FF; /* 25LC040 has 512 bytes */
    ext_eeprom_write_enable();

    ee_cs_low();
    spi_transfer((uint8_t)(EE25_WRITE | ((addr & 0x0100) ? 0x08 : 0x00)));
    spi_transfer((uint8_t)(addr & 0xFF));
    spi_transfer(data);
    ee_cs_high();

    ext_eeprom_wait_ready();
}

uint8_t ext_eeprom_read_byte(uint16_t addr)
{
    uint8_t data;
    addr &= 0x01FF;

    ee_cs_low();
    spi_transfer((uint8_t)(EE25_READ | ((addr & 0x0100) ? 0x08 : 0x00)));
    spi_transfer((uint8_t)(addr & 0xFF));
    data = spi_transfer(0xFF);
    ee_cs_high();

    return data;
}
