#ifndef PLAYER_H
#define PLAYER_H

typedef struct {
    uint16_t dt;  // [ms]
    uint8_t cmd;
    uint8_t data_a;
    uint8_t data_b;
} t_midi_event;

void play_file(const char *fn);

void stop_playback();

void refresh_player();

#endif
