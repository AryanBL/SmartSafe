#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>

void logger_init(void);
void log_event(uint8_t event_code, uint32_t timestamp);
void log_event_data(uint8_t event_code, uint32_t timestamp, uint16_t data);
void log_dump(void);
void log_clear(void);

#endif /* LOGGER_H */
