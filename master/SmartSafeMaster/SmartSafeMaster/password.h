#ifndef PASSWORD_H
#define PASSWORD_H

#include <stdint.h>

void password_init(void);
uint8_t password_check(const char *entered4);
void password_change(const char *new_pass4);
uint8_t password_get_fail_count(void);
uint8_t password_increment_fail(void);
void password_reset_fail(void);

#endif /* PASSWORD_H */
