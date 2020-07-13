#ifndef MUSICAL_H
#define MUSICAL_H

// number of max. simultaneous tones
// esp866 sounds drunk when > 2
#define N_VOICES 2

void all_off();

void init_musical();

void refresh_musical();

#endif
