#include "config.h"
#include "adc.h"
#include "tamper.h"

#define TAMPER_REARM_BAND       5U
#define TAMPER_STABLE_SAMPLES   3U

static uint16_t reference_adc = 0;
static uint8_t armed = 1;
static uint8_t stable_samples = 0;

static uint16_t adc_difference(uint16_t a, uint16_t b)
{
    return (a >= b) ? (a - b) : (b - a);
}

void tamper_init(void)
{
    /*
     * Use the current potentiometer position as the initial stable
     * reference when the system starts or returns from power standby.
     */
    reference_adc = adc_read(ADC_TAMPER_CH);
    armed = 1;
    stable_samples = 0;
}

uint8_t tamper_check(void)
{
    uint16_t value = adc_read(ADC_TAMPER_CH);
    uint16_t difference = adc_difference(value, reference_adc);

    if (armed) {
        /*
         * Do not continuously replace reference_adc here.
         * This allows several small manual changes to accumulate.
         */
        if (difference > TAMPER_THRESHOLD) {
            armed = 0;
            stable_samples = 0;

            /*
             * The new potentiometer position becomes the reference
             * used while waiting for the sensor to stabilize.
             */
            reference_adc = value;

            return 1;
        }

        return 0;
    }

    /*
     * After a tamper event, wait until the sensor remains stable
     * for three consecutive 100 ms samples before rearming.
     */
    difference = adc_difference(value, reference_adc);

    if (difference <= TAMPER_REARM_BAND) {
        if (++stable_samples >= TAMPER_STABLE_SAMPLES) {
            armed = 1;
            stable_samples = 0;
        }
    } else {
        /*
         * The potentiometer is still moving. Follow its new position,
         * then restart the stability counter.
         */
        reference_adc = value;
        stable_samples = 0;
    }

    return 0;
}