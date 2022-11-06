#ifndef PULSER_H
#define PULSER_H

// I2S config
#define BITS_PER_SEC 500000  // smallest pulse: 2 us
#define WORDS_PER_SEC (BITS_PER_SEC / 32)

#define TC_ON() digitalWrite(PIN_FIRE, !IS_ACTIVE_LOW)  // enable TC
#define TC_OFF() digitalWrite(PIN_FIRE, IS_ACTIVE_LOW)  // disable TC

extern unsigned g_t_pre;  // [us]
extern unsigned g_t_on;  // [us]
extern unsigned g_t_off;  // [us]

void init_pulser(void);
void refresh_pulser(void);
void stop_pulse(void);
void set_pulse(unsigned v, unsigned ftw, unsigned duty);
void set_phase(unsigned v, unsigned p);

void set_single_shot(unsigned pw_us);


void dds_mode();
void lock_mode();
void single_shot_mode();

#endif
