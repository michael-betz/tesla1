#include <Arduino.h>
#include <i2s.h>
#include <i2s_reg.h>
#include "pulser.h"

static unsigned g_ftw;   // 32 bit DDS frequency tuning word
static unsigned g_duty;  // 32 bit DDS duty cycle (0 = OFF, 0xFFFFFFFF = CW)
static unsigned g_phase; // 32 bit DDS phase

// Use Direct Digital Synthesis to get a pulse waveform with precisely
// adjustable (32 bit resolution) frequency, duty cycle and phase
// (relative to an arbitrary starting point).
// This also dithers the output nicely to achieve precise duty cycles
// and fractional frequencies (on average).
void refresh_pulser(void)
{
	static unsigned acc = 0;  // 32 bit phase accumulator
	static unsigned sample = IS_ACTIVE_LOW ? 0xFFFFFFFF : 0;

	// try to output the previously missed sample again
	if (i2s_write_sample_nb(sample) == false) return;

	// Generate N new samples to fill up the DMA buffer
	while (1) {
		for (unsigned i=0; i<=31; i++) {
			sample <<= 1;
			acc += g_ftw;
			unsigned tmp = acc + g_phase;
			if (IS_ACTIVE_LOW) {
				if (tmp >= g_duty)
					sample |= 1;
			} else {
				if (tmp < g_duty)
					sample |= 1;
			}
		}

		// Stop generating new samples on buffer overflow
		// the current sample will be output on beginning of next call
		if (i2s_write_sample_nb(sample) == false) return;
	}
}

void stop_pulse(void) {	g_duty = 0; }

void set_phase(unsigned p) { g_phase = p; }

void set_pulse(unsigned ftw, unsigned duty)
{
	// Max. ON time limit
	unsigned t_on = duty / ftw;  				// [cycles]
	if (t_on > (1ULL * MAX_T_ON * BITS_PER_SEC / 1000000)) {
		Serial.printf("exceeds MAX_T_ON = %d us\n", MAX_T_ON);
		return;
	}

	// Max. duty cycle limit
	if (duty > (0xFFFFFFFFULL * MAX_DUTY_PERCENT / 100)) {
		Serial.printf("exceeds MAX_DUTY_PERCENT = %d %%\n", MAX_DUTY_PERCENT);
		return;
	}

	g_ftw = ftw;
	g_duty = duty;
	Serial.printf("ftw: %08x  duty: %08x\n", g_ftw, g_duty);
}

void init_pulser(void)
{
	g_ftw = 0;
	g_duty = 0;
	g_phase = 0;

	i2s_begin();
	i2s_set_rate(WORDS_PER_SEC);
}
