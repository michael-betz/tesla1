#ifndef MUSICAL_H
#define MUSICAL_H

// number of max. simultaneous tones
// too many and the esp866 gets laggy and sounds drunk
// N_VOICES = 2 works well for BITS_PER_SEC = 1000000
// N_VOICES = 3 works still okay for BITS_PER_SEC =  500000
// but loads down the esp quite a bit and sounds like noise anyway
#define N_VOICES 3

void all_off();

void init_musical();

void refresh_musical();

#endif
