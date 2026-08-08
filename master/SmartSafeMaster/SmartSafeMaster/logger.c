#include "config.h"
#include "spi_eeprom.h"
#include "usart.h"
#include "logger.h"
#include <avr/wdt.h>

static uint8_t log_head = 0;   /* next write index */
static uint8_t log_count = 0;  /* valid record count */

static uint8_t checksum7(const uint8_t *b)
{
    uint8_t x = 0;
    for (uint8_t i = 0; i < 7; i++)
        x ^= b[i];
    return x;
}

static void write_header(void)
{
    ext_eeprom_write_byte(LOG_HEADER_ADDR + 0, LOG_MAGIC0);
    ext_eeprom_write_byte(LOG_HEADER_ADDR + 1, LOG_MAGIC1);
    ext_eeprom_write_byte(LOG_HEADER_ADDR + 2, log_head);
    ext_eeprom_write_byte(LOG_HEADER_ADDR + 3, log_count);
    ext_eeprom_write_byte(LOG_HEADER_ADDR + 4, 0xFF);
    ext_eeprom_write_byte(LOG_HEADER_ADDR + 5, 0xFF);
    ext_eeprom_write_byte(LOG_HEADER_ADDR + 6, 0xFF);
    ext_eeprom_write_byte(LOG_HEADER_ADDR + 7, 0xFF);
}

void log_clear(void)
{
    log_head = 0;
    log_count = 0;
    write_header();
}

void logger_init(void)
{
    uint8_t m0 = ext_eeprom_read_byte(LOG_HEADER_ADDR + 0);
    uint8_t m1 = ext_eeprom_read_byte(LOG_HEADER_ADDR + 1);

    if (m0 != LOG_MAGIC0 || m1 != LOG_MAGIC1) {
        log_clear();
        return;
    }

    log_head = ext_eeprom_read_byte(LOG_HEADER_ADDR + 2);
    log_count = ext_eeprom_read_byte(LOG_HEADER_ADDR + 3);

    if (log_head >= LOG_MAX_RECORDS || log_count > LOG_MAX_RECORDS)
        log_clear();
}

void log_event(uint8_t event_code, uint32_t timestamp)
{
    log_event_data(event_code, timestamp, 0xFFFF);
}

void log_event_data(uint8_t event_code, uint32_t timestamp, uint16_t data)
{
    uint8_t r[LOG_RECORD_SIZE];
    uint16_t base;

    if (log_head >= LOG_MAX_RECORDS)
        log_head = 0;

    r[0] = event_code;
    r[1] = (uint8_t)(timestamp >> 24);
    r[2] = (uint8_t)(timestamp >> 16);
    r[3] = (uint8_t)(timestamp >> 8);
    r[4] = (uint8_t)(timestamp);
    r[5] = (uint8_t)(data >> 8);
    r[6] = (uint8_t)(data);
    r[7] = checksum7(r);

    base = LOG_BASE_ADDR + (uint16_t)log_head * LOG_RECORD_SIZE;
    for (uint8_t i = 0; i < LOG_RECORD_SIZE; i++)
        ext_eeprom_write_byte(base + i, r[i]);

    log_head++;
    if (log_head >= LOG_MAX_RECORDS)
        log_head = 0;

    if (log_count < LOG_MAX_RECORDS)
        log_count++;

    write_header();
}

static void print_uint32(uint32_t val)
{
    char buf[11];
    uint8_t i = 10;
    buf[10] = '\0';

    if (val == 0) {
        usart_tx('0');
        return;
    }

    while (val && i > 0) {
        buf[--i] = (char)('0' + (val % 10));
        val /= 10;
    }
    usart_print(&buf[i]);
}

static void print_hex8(uint8_t v)
{
    static const char hex[] = "0123456789ABCDEF";
    usart_tx(hex[v >> 4]);
    usart_tx(hex[v & 0x0F]);
}

static const char *event_name(uint8_t code)
{
    switch (code) {
    case EVT_LOGIN_OK:   return "LOGIN_OK";
    case EVT_LOGIN_FAIL: return "LOGIN_FAIL";
    case EVT_LOCKOUT:    return "LOCKOUT";
    case EVT_TAMPER:     return "TAMPER";
    case EVT_POWER_FAIL: return "POWER_FAIL";
    case EVT_POWER_OK:   return "POWER_OK";
    case EVT_WDT_RESET:  return "WDT_RESET";
    case EVT_SLEEP:      return "SLEEP";
    case EVT_WAKE:       return "WAKE";
    default:             return "UNKNOWN";
    }
}

void log_dump(void)
{
    usart_print("=== LOG DUMP ===\r\n");
    usart_print("COUNT=");
    print_uint32(log_count);
    usart_print(" HEAD=");
    print_uint32(log_head);
    usart_print("\r\n");

    uint8_t start = (log_count == LOG_MAX_RECORDS) ? log_head : 0;

    for (uint8_t n = 0; n < log_count; n++) {
        wdt_reset();
        uint8_t idx = (uint8_t)((start + n) % LOG_MAX_RECORDS);
        uint16_t base = LOG_BASE_ADDR + (uint16_t)idx * LOG_RECORD_SIZE;
        uint8_t r[LOG_RECORD_SIZE];

        for (uint8_t i = 0; i < LOG_RECORD_SIZE; i++)
            r[i] = ext_eeprom_read_byte(base + i);

        if (checksum7(r) != r[7]) {
            usart_print("IDX=");
            print_uint32(idx);
            usart_print(" BAD_CHECKSUM\r\n");
            continue;
        }

        uint32_t ts = ((uint32_t)r[1] << 24) |
                      ((uint32_t)r[2] << 16) |
                      ((uint32_t)r[3] << 8)  |
                      ((uint32_t)r[4]);
        uint16_t data = ((uint16_t)r[5] << 8) | r[6];

        usart_print("IDX=");
        print_uint32(idx);
        usart_print(" EVT=0x");
        print_hex8(r[0]);
        usart_print(" ");
        usart_print(event_name(r[0]));
        usart_print(" TS=");
        print_uint32(ts);
        usart_print("s DATA=0x");
        print_hex8((uint8_t)(data >> 8));
        print_hex8((uint8_t)data);
        usart_print("\r\n");
    }

    usart_print("=== END ===\r\n");
}
