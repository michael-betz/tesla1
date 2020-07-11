#include <Arduino.h>
#include <i2s.h>
#include <i2s_reg.h>
#include "pulser.h"

enum {
	NO_LOCK,
	LOCK_OPTO,
	LOCK_PRE,
	LOCK_ON,
	LOCK_OFF
} g_isr_state;

// 2 operating modes:
//   * free running DDS mode, which gives precise frequency and duty cycle control
void dds_mode()
{
	timer1_disable();
	g_isr_state = NO_LOCK;
	stop_pulse();
	pinMode(PIN_FIRE, FUNCTION_1);
}

// 	 * 60 Hz locked mode, which uses interrupts to lock to mains frequency
void lock_mode()
{
	digitalWrite(PIN_FIRE, IS_ACTIVE_LOW); // disable TC
	pinMode(PIN_FIRE, OUTPUT);
	timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
	// g_t_on = 0;
	g_isr_state = LOCK_OPTO;
}

// -------------------------------
//  DDS free-running mode
// -------------------------------
static unsigned g_ftw[N_VOICES]; // 32 bit DDS frequency tuning word
static unsigned g_duty[N_VOICES]; // 32 bit DDS duty cycle (0 = OFF, 0xFFFFFFFF = CW)
static unsigned g_phase[N_VOICES]; // 32 bit DDS phase

// Use Direct Digital Synthesis to get a pulse waveform with precisely
// adjustable (32 bit resolution) frequency, duty cycle and phase
// (relative to an arbitrary starting point).
// This also dithers the output nicely to achieve precise duty cycles
// and fractional frequencies (on average).
void refresh_pulser(void)
{
	static unsigned acc[N_VOICES];  // 32 bit phase accumulator
	static unsigned sample = IS_ACTIVE_LOW ? 0xFFFFFFFF : 0;

	// Try to output the previously missed sample, then
	// generate N new samples until the DMA buffer is full
	while (i2s_write_sample_nb(sample)) {
		sample = 0;
		for (unsigned i=0; i<=31; i++) {
			bool bit = IS_ACTIVE_LOW;
			for (unsigned v=0; v<N_VOICES; v++) {
				acc[v] += g_ftw[v];
				bit ^= (acc[v] + g_phase[v]) < g_duty[v];
			}
			sample <<= 1;
			sample |= bit;
		}
	}
}

void stop_pulse(void)
{
	for (unsigned v=0; v<N_VOICES; v++) {
		g_duty[v] = 0;
	}
}

void set_phase(unsigned v, unsigned p)
{
	if (v >= N_VOICES) return;
	Serial.printf("phase[%d]: %08x\n", v, p);
	g_phase[v] = p;
}

void set_pulse(unsigned v, unsigned ftw, unsigned duty)
{
	if (v >= N_VOICES) return;

	// Max. ON time limit
	if (duty > 0) {
		unsigned t_on = duty / ftw;  // [cycles]
		if (t_on > (1ULL * MAX_T_ON * BITS_PER_SEC / 1000000)) {
			Serial.printf("exceeds MAX_T_ON = %d us\n", MAX_T_ON);
			return;
		}
	}

	// Max. duty cycle limit
	if (duty > (0xFFFFFFFFULL * MAX_DUTY_PERCENT / 100)) {
		Serial.printf("exceeds MAX_DUTY_PERCENT = %d %%\n", MAX_DUTY_PERCENT);
		return;
	}

	g_ftw[v] = ftw;
	g_duty[v] = duty;
	Serial.printf("ftw[%d]: %08x  duty: %08x\n", v, ftw, duty);
}

// -------------------------------
//  60 Hz locking mode
// -------------------------------
// states:
//   * wait for opto rising edge, start timer1 for t_pre
//   * enable TC, start timer1 for t_on
//   * disable TC, repeat

unsigned g_t_pre = 10;  // [us]
unsigned g_t_on = 0;  // [us]
unsigned g_t_off = 300000;  // [us]

ICACHE_RAM_ATTR void timer_isr()
{
	switch (g_isr_state) {
		case LOCK_PRE:
			g_isr_state = LOCK_ON;
			if (g_t_on > 0) {
				digitalWrite(PIN_FIRE, !IS_ACTIVE_LOW); // enable TC
				timer1_write(g_t_on * 5);
			} else {
				digitalWrite(PIN_FIRE, IS_ACTIVE_LOW); // disable TC
				g_isr_state = LOCK_OPTO;
			}
			break;

		case LOCK_ON:
			g_isr_state = LOCK_OFF;
			digitalWrite(PIN_FIRE, IS_ACTIVE_LOW); // disable TC
			// timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
			timer1_write(g_t_off * 5);
			break;

		case LOCK_OFF:
			g_isr_state = LOCK_OPTO;
			break;

		default:
			digitalWrite(PIN_FIRE, IS_ACTIVE_LOW); // disable TC
			g_isr_state = NO_LOCK;
			timer1_disable();
	}
}

ICACHE_RAM_ATTR void opto_isr()
{
	static unsigned last_millis = 0;
	unsigned m = millis();

	if (m - last_millis >= 10) {
		last_millis = m;
		// sync_pulser();
		if (g_isr_state == LOCK_OPTO) {
			g_isr_state = LOCK_PRE;
			timer1_write(g_t_pre * 5);
		}
	}
}

void init_pulser(void)
{
	for (unsigned v=0; v<N_VOICES; v++) {
		g_ftw[v] = 0;
		g_duty[v] = 0;
		g_phase[v] = 0;
	}

	i2s_begin();
	i2s_set_rate(WORDS_PER_SEC);

	dds_mode();

	pinMode(PIN_60HZ, INPUT);
	attachInterrupt(digitalPinToInterrupt(PIN_60HZ), opto_isr, RISING);
	timer1_attachInterrupt(timer_isr);
}
