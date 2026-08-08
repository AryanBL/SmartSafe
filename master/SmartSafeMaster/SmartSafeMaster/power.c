#include <avr/io.h>
#include <avr/interrupt.h>
#include "config.h"
#include "power.h"
#include "timer.h"

/* These represent the accepted, software-filtered power state. */
volatile uint8_t power_event_pending = 0;
volatile uint8_t power_fail_state = 0;

/* These variables belong to the software stability filter. */
static volatile uint8_t power_candidate_state = 0;
static volatile uint8_t power_confirm_ticks = 0;
static volatile uint8_t power_confirmation_pending = 0;

static uint8_t comparator_reports_power_fail(void)
{
    /*
     * ACBG=1 connects the internal bandgap to comparator V+.
     * The divided monitored supply is connected to AIN1/PB3, comparator V-.
     * Therefore ACO=1 means Vbandgap > Vdivider, interpreted as power fail.
     */
    return (ACSR & (1 << ACO)) ? 1U : 0U;
}

ISR(ANA_COMP_vect)
{
    /*
     * While the normal system is running, every output transition restarts
     * the software stability timer.  During power-failure standby Timer2 is
     * stopped; this ISR still wakes the MCU when ACO toggles on restoration.
     */
    power_candidate_state = comparator_reports_power_fail();
    power_confirm_ticks = POWER_CONFIRM_TICKS_10MS;
    power_confirmation_pending = 1;
}

void power_monitor_tick_10ms(void)
{
    if (power_confirmation_pending && power_confirm_ticks > 0)
        power_confirm_ticks--;
}

void power_monitor_process(void)
{
    uint8_t candidate;
    uint8_t sreg;

    sreg = SREG;
    cli();

    if (!power_confirmation_pending || power_confirm_ticks != 0) {
        SREG = sreg;
        return;
    }

    candidate = power_candidate_state;
    power_confirmation_pending = 0;

    SREG = sreg;

    /* Reject a candidate that no longer agrees with the live comparator. */
    if (comparator_reports_power_fail() != candidate)
        return;

    /* Report only a real accepted-state transition. */
    if (candidate != power_fail_state) {
        power_fail_state = candidate;
        power_event_pending = 1;
    }
}

void power_monitor_suspend_for_sleep(void)
{
    uint8_t sreg = SREG;
    cli();

    /* Cancel an unfinished confirmation and disable comparator interrupts. */
    power_confirm_ticks = 0;
    power_confirmation_pending = 0;

    /* ACBG remains enabled; ACIE=0. ACI is cleared by writing a logic one. */
    ACSR = (1 << ACBG) | (1 << ACI);

    SREG = sreg;
}

void power_monitor_resume_after_sleep(void)
{
    uint8_t current;
    uint8_t sreg;

    /* Clear any stale flag, then enable toggle interrupts again. */
    ACSR = (1 << ACBG) | (1 << ACI);
    ACSR = (1 << ACBG) | (1 << ACIE);

    current = comparator_reports_power_fail();

    sreg = SREG;
    cli();

    if (current != power_fail_state) {
        power_candidate_state = current;
        power_confirm_ticks = POWER_CONFIRM_TICKS_10MS;
        power_confirmation_pending = 1;
    } else {
        power_candidate_state = current;
        power_confirm_ticks = 0;
        power_confirmation_pending = 0;
    }

    SREG = sreg;
}

void power_monitor_prepare_restore_wait(void)
{
    uint8_t sreg = SREG;
    cli();

    /* The failure event has already been accepted and processed. */
    power_candidate_state = 1U;
    power_confirm_ticks = 0;
    power_confirmation_pending = 0;

    /* Comparator toggle interrupt is the only interrupt retained in standby. */
    ACSR = (1 << ACBG) | (1 << ACI);
    ACSR = (1 << ACBG) | (1 << ACIE);

    SREG = sreg;
}

uint8_t power_monitor_raw_failed(void)
{
    return comparator_reports_power_fail();
}

uint8_t power_monitor_accept_restore_if_present(void)
{
    uint8_t restored = 0;
    uint8_t sreg = SREG;

    cli();

    /* Recheck ACO inside the atomic section before accepting restoration. */
    if (!comparator_reports_power_fail()) {
        power_fail_state = 0;
        power_event_pending = 1;
        power_candidate_state = 0;
        power_confirm_ticks = 0;
        power_confirmation_pending = 0;
        restored = 1;
    }

    SREG = sreg;
    return restored;
}

void power_monitor_init(void)
{
    uint8_t initial_state;

    /* PB3/AIN1 is the dedicated comparator negative input. */
    POWER_SENSE_DDR &= ~(1U << POWER_SENSE_PIN);
    POWER_SENSE_PORT &= ~(1U << POWER_SENSE_PIN);

    /*
     * Explicitly use the dedicated AIN1 pin rather than the ADC
     * multiplexer as the comparator negative input.
     */
    #ifdef ACME
        SFIOR &= ~(1U << ACME);
    #endif

    /*
     * Enable the comparator with the internal bandgap connected
     * to its positive input. Keep its interrupt disabled for now.
     */
    ACSR = (1U << ACBG);

    /*
     * The bandgap startup time is at most approximately 70 us.
     * One millisecond provides substantial margin and also works
     * reliably in Proteus.
     */
    delay_ms(1);

    /* Read the comparator only after the reference has stabilized. */
    initial_state = comparator_reports_power_fail();

    power_fail_state = initial_state;
    power_candidate_state = initial_state;

    /*
     * If power is already low at startup or after an external reset,
     * create a startup failure event even though no toggle occurred.
     */
    power_event_pending = initial_state ? 1U : 0U;

    power_confirm_ticks = 0;
    power_confirmation_pending = 0;

    /* Clear a stale comparator flag, then enable toggle interrupts. */
    ACSR = (1U << ACBG) | (1U << ACI);
    ACSR = (1U << ACBG) | (1U << ACIE);
}
