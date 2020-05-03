#include <Arduino.h>
#include <i2s.h>
#include <i2s_reg.h>
#include "pulser.h"

static unsigned t_on = 0;  // [us]
static unsigned t_off = 1000000;  // 1 s

void refresh_pulser(void)
{
	static unsigned pulse_state=0, bit_counter=0;
	static unsigned sample=0;

	// Output the previously missed sample again
	if (i2s_write_sample_nb(sample) == false) return;

	// Generate N new samples
	while (1) {
		if (t_on == 0) {
			sample = 0;
		} else if (t_off == 0) {
			sample = 0xFFFFFFFF;
		} else {
			for (unsigned i=0; i<=31; i++) {
				if (bit_counter <= 0) {
					if (pulse_state) {
						pulse_state = 0;
						bit_counter = t_off;
					} else {
						pulse_state = 1;
						bit_counter = t_on;
					}
				}
				sample <<= 1;
				sample |= pulse_state & 1;
				bit_counter--;
			}
		}

		// Stop generating new samples on buffer overflow
		// the current sample will be output on beginning of next call
		if (i2s_write_sample_nb(sample) == false) return;
	}
}

void stop_pulse(void)
{
	t_on = 0;
	t_off = 1000000;
}

void set_pulse(int temp_on, int temp_off)
{
	Serial.printf("t_on = %d, t_off = %d ", temp_on, temp_off);

	if (temp_on > MAX_T_ON) {
		Serial.printf("exceeds MAX_T_ON = %d us\n", MAX_T_ON);
		return;
	}

	// duty cycle limit
	if (100 * temp_on / MAX_DUTY_PERCENT > temp_off) {
		Serial.printf(
			"exceeds MAX_DUTY_PERCENT = %d %%\n", MAX_DUTY_PERCENT
		);
		return;
	}
	t_on = temp_on;
	t_off = temp_off;
	Serial.print(" done\n");
}

void init_pulser(void)
{
	i2s_begin();
	i2s_set_rate(WORDS_PER_SEC);
}
