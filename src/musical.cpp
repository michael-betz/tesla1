#include "WiFiUdp.h"
#include "ESP8266mDNS.h"
#include "AppleMIDI.h"
#include "pulser.h"
#include "musical.h"

USING_NAMESPACE_APPLEMIDI
APPLEMIDI_CREATE_INSTANCE(WiFiUDP, MIDI, HOST_NAME, DEFAULT_CONTROL_PORT);

static unsigned const midi_ftws[128] = {
    0x00004495, 0x000048a9, 0x00004cfb, 0x0000518f,
    0x00005668, 0x00005b8c, 0x000060fd, 0x000066c2,
    0x00006cde, 0x00007357, 0x00007a33, 0x00008177,
    0x0000892a, 0x00009152, 0x000099f7, 0x0000a31e,
    0x0000acd1, 0x0000b718, 0x0000c1fb, 0x0000cd84,
    0x0000d9bd, 0x0000e6af, 0x0000f467, 0x000102ef,
    0x00011255, 0x000122a5, 0x000133ee, 0x0001463d,
    0x000159a3, 0x00016e31, 0x000183f7, 0x00019b09,
    0x0001b37a, 0x0001cd5f, 0x0001e8ce, 0x000205df,
    0x000224ab, 0x0002454b, 0x000267dc, 0x00028c7b,
    0x0002b347, 0x0002dc62, 0x000307ef, 0x00033612,
    0x000366f5, 0x00039abf, 0x0003d19d, 0x00040bbf,
    0x00044956, 0x00048a96, 0x0004cfb8, 0x000518f6,
    0x0005668e, 0x0005b8c4, 0x00060fde, 0x00066c25,
    0x0006cdea, 0x0007357e, 0x0007a33b, 0x0008177f,
    0x000892ac, 0x0009152d, 0x00099f70, 0x000a31ec,
    0x000acd1d, 0x000b7189, 0x000c1fbc, 0x000cd84b,
    0x000d9bd4, 0x000e6afc, 0x000f4677, 0x00102efe,
    0x00112559, 0x00122a5a, 0x00133ee0, 0x001463d8,
    0x00159a3b, 0x0016e313, 0x00183f79, 0x0019b097,
    0x001b37a8, 0x001cd5f9, 0x001e8cee, 0x00205dfc,
    0x00224ab2, 0x002454b4, 0x00267dc1, 0x0028c7b0,
    0x002b3477, 0x002dc627, 0x00307ef3, 0x0033612f,
    0x00366f50, 0x0039abf3, 0x003d19dc, 0x0040bbf9,
    0x00449564, 0x0048a969, 0x004cfb82, 0x00518f60,
    0x005668ee, 0x005b8c4f, 0x0060fde7, 0x0066c25e,
    0x006cdea0, 0x007357e6, 0x007a33b8, 0x008177f2,
    0x00892ac9, 0x009152d2, 0x0099f704, 0x00a31ec1,
    0x00acd1dc, 0x00b7189e, 0x00c1fbcf, 0x00cd84bc,
    0x00d9bd40, 0x00e6afcc, 0x00f46770, 0x0102efe5,
    0x01125593, 0x0122a5a5, 0x0133ee09, 0x01463d83,
    0x0159a3b9, 0x016e313d, 0x0183f79f, 0x019b0979,
};

static int cur_notes[N_VOICES];
static uint8_t note_fifo[N_VOICES];

unsigned get_voice()
{
    // find IDLE DDS channel
    for (unsigned v=0; v<N_VOICES; v++)
        if (cur_notes[v] < 0)
            return v;
    // nothiung is IDLE, overwrite oldest note
    return note_fifo[N_VOICES - 1];
}

byte g_volume = 100;

void note_on(byte channel, byte note, byte velocity)
{
    if (note >= 128)
        return;

    unsigned v_ch = get_voice();

    for (unsigned v=N_VOICES-1; v>0; v--)
        note_fifo[v] = note_fifo[v - 1];
    note_fifo[0] = v_ch;

    cur_notes[v_ch] = note;

    set_pulse(v_ch, midi_ftws[note], (velocity * velocity * g_volume) << 8);

    // Serial.printf("+[%d]: %d, %d\n", v_ch, note, velocity);
}

void note_off(byte channel, byte note, byte velocity)
{
    if (note >= 128)
        return;

    for (unsigned v=0; v<N_VOICES; v++) {
        if (note == cur_notes[v]) {
            set_pulse(v, 0, 0);
            // Serial.printf("-[%d]: %d, %d\n", v, note, velocity);
            cur_notes[v] = -1;
        }
    }
}

void pitch_bend(byte channel, int bend)
{
    // todo
}

void midi_msg(const midi::Message<128u>& msg)
{
    Serial.printf("M: %x\n", msg.type);
}

void all_off()
{
    for (unsigned v=0; v<N_VOICES; v++) {
        cur_notes[v] = -1;
        note_fifo[v] = v;
    }
    stop_pulse();
}

void ctrl_change(byte channel, byte number, byte value)
{
    switch (number) {
        // case 0:  // B. buttons
        // case 1:  // Modulation
        // case 2: // Breath
        case 7: // Volume
            all_off();
            g_volume = value;
            break;
        // case 12 - 29:  // organ trim registers
        // case 71:  // pad-x
        // case 74:  // pad-y
        case 120:
        case 121:
        case 122:
        case 123:
        case 124:
        case 125:
        case 126:
        case 127:
            all_off();

        default:
            Serial.printf("CTRL: %d, %d, %d\n", channel, number, value);
    }
}

void midi_con(const ssrc_t & ssrc, const char* name)
{
    Serial.printf("midi_con(%s)\n", name);
}

void midi_discon(const ssrc_t & ssrc)
{
    Serial.println("midi_discon()\n");
    all_off();
}

void midi_err(const ssrc_t& ssrc, int32_t err)
{
    Serial.printf("midi_err(%x)\n", err);
    all_off();
}

void init_musical()
{
    // listen on midi channel 1 for events
    MIDI.begin(1);
    MIDI.setHandleNoteOn(note_on);
    MIDI.setHandleNoteOff(note_off);
    MIDI.setHandleMessage(midi_msg);
    MIDI.setHandleControlChange(ctrl_change);
    // MIDI.setHandlePitchBend(pitch_bend);
    AppleMIDI.setHandleConnected(midi_con);
    AppleMIDI.setHandleDisconnected(midi_discon);
    AppleMIDI.setHandleError(midi_err);

    MDNS.begin(HOST_NAME);
    MDNS.addService("apple-midi", "udp", AppleMIDI.getPort());
    Serial.printf("Listening on UDP:%d for rtp midi\n", AppleMIDI.getPort());
}


void refresh_musical()
{
    MIDI.read();
}
