#include "LittleFS.h"
#include "pulser.h"
#include "musical.h"
#include "player.h"


static bool is_playing = false;
static File midi_file;
static uint32_t ms_ = 0;
static t_midi_event next_evt;


static int get_next_event()
{
    if (!midi_file)
        return -1;

    // get next note
    int ret = midi_file.read((uint8_t*)&next_evt, sizeof(t_midi_event));
    if (ret != sizeof(t_midi_event))
        return -1;

    return ret;
}

static void play_event()
{
    uint8_t ch = next_evt.cmd & 0x0F;
    uint8_t cmd = (next_evt.cmd >> 4) & 0x0F;

    switch(cmd) {
        case 0x8:
            note_off(ch, next_evt.data_a, next_evt.data_b);
            break;

        case 0x9:
            note_on(ch, next_evt.data_a, next_evt.data_b);
            break;

        case 0xE:
            pitch_bend(ch, (next_evt.data_b << 7) | (next_evt.data_a & 0x7F));
            break;
    }
}

void refresh_player()
{
    if (!is_playing)
        return;

    uint32_t ms = millis();

    if ((ms - ms_) >= next_evt.dt) {
        play_event();
        ms_ = ms;

        if (get_next_event() < 0) {
            stop_playback();
        }
    }
}


void stop_playback()
{
    is_playing = false;
    all_off();
    if (midi_file)
        midi_file.close();
}


void play_file(const char *fn)
{
    stop_playback();

    midi_file = LittleFS.open(fn, "r");
    if (!midi_file) {
        Serial.printf("Failed to open %s", fn);
        return;
    }

    if (get_next_event() < 0) {
        return;
    }

    ms_ = millis();
    is_playing = true;
}
