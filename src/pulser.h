#ifndef PULSER_H
#define PULSER_H

// I2S config
#define BITS_PER_SEC 1000000  // 1 Mbits/s
#define WORDS_PER_SEC (BITS_PER_SEC / 32)

extern unsigned g_t_pre;  // [us]
extern unsigned g_t_on;  // [us]
extern unsigned g_t_off;  // [us]

void init_pulser(void);
void refresh_pulser(void);
void stop_pulse(void);
void set_pulse(unsigned v, unsigned ftw, unsigned duty);
void set_phase(unsigned v, unsigned p);

void dds_mode();
void lock_mode();

#endif
