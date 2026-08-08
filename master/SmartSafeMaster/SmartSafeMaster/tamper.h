#ifndef TAMPER_H
#define TAMPER_H

#include <stdint.h>

void tamper_init(void);
uint8_t tamper_check(void); /* returns 1 only on a new tamper event */

#endif /* TAMPER_H */
