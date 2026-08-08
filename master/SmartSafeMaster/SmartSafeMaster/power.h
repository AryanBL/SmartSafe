#ifndef POWER_H
#define POWER_H

#include <stdint.h>

/* Accepted, software-filtered power state and event flag used by main.c. */
extern volatile uint8_t power_event_pending;
extern volatile uint8_t power_fail_state;

void power_monitor_init(void);

/* Called from the Timer2 compare ISR once every approximately 10 ms. */
void power_monitor_tick_10ms(void);

/* Called repeatedly from the main loop to accept a state only after it is stable. */
void power_monitor_process(void);

/*
 * Inactivity Power-down uses INT0 as its only wake source.  These functions
 * temporarily disable the comparator interrupt and resynchronize the current
 * comparator state after INT0 wakes the MCU.
 */
void power_monitor_suspend_for_sleep(void);
void power_monitor_resume_after_sleep(void);

/*
 * Power-failure standby keeps only the analog comparator interrupt active.
 * The MCU sleeps in Idle until the raw comparator state reports restoration.
 */
void power_monitor_prepare_restore_wait(void);
uint8_t power_monitor_raw_failed(void);
uint8_t power_monitor_accept_restore_if_present(void);

#endif /* POWER_H */
