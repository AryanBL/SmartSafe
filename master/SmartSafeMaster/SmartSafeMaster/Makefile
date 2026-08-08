MCU=atmega32
F_CPU=8000000UL
CC=avr-gcc
OBJCOPY=avr-objcopy
CFLAGS=-mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -Wall -Wextra -std=gnu99

TARGET=smartsafe_master
SRCS=main.c LCD.c timer.c usart.c keypad.c servo.c adc.c tamper.c spi_eeprom.c logger.c password.c power.c
OBJS=$(SRCS:.c=.o)

all: $(TARGET).hex

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).hex
