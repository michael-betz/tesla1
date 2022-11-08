#include "WiFiUdp.h"
#include "ESP8266mDNS.h"
#include "AppleMIDI.h"
#include "LittleFS.h"
#include "pulser.h"
#include "musical.h"

USING_NAMESPACE_APPLEMIDI
APPLEMIDI_CREATE_INSTANCE(WiFiUDP, MIDI, HOST_NAME, DEFAULT_CONTROL_PORT);

// frequency tuning words for all 128 midi notes
// generated by midi_ftws.py
static unsigned const midi_ftws[128] = {
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
    0x01b37a81, 0x01cd5f99, 0x01e8cee1, 0x0205dfca,
    0x0224ab27, 0x02454b4a, 0x0267dc12, 0x028c7b07,
    0x02b34772, 0x02dc627a, 0x0307ef3e, 0x033612f2,
};

static int cur_notes[N_VOICES];
static int cur_duty[N_VOICES];
static unsigned note_ts[N_VOICES];  // timestamp [ms]

static unsigned get_voice()
{
    unsigned min_ts=note_ts[0], min_ts_ind=0;
    for (unsigned v=0; v<N_VOICES; v++) {
        // find IDLE DDS channel
        if (cur_notes[v] < 0)
            return v;
        // find smallest timestamp = oldest note
        if (note_ts[v] < min_ts) {
            min_ts = note_ts[v];
            min_ts_ind = v;
        }
    }
    // nothing is IDLE, overwrite oldest note
    return min_ts_ind;
}

static void print_voices()
{
    for (unsigned v=0; v<N_VOICES; v++) {
        if (cur_notes[v] < 0 || cur_duty[v] < 0)
            Serial.print(" . ");
        else
            Serial.printf("%2x ", cur_notes[v]);
    }
    Serial.print('\n');
}

static unsigned get_bend_ftw(unsigned ftw, int bend)
{
    // this might be a bit expensive ...
    float new_ftw = pow(2.0f, (float)bend / 8192.0f) * ftw;
    return new_ftw;
}

int g_volume = 100;
int g_cur_bend = 0;
unsigned g_b_button = 0;

static void note_on(byte channel, byte note, byte velocity)
{
    if (note >= 128)
        return;

    unsigned v_next = get_voice();
    note_ts[v_next] = millis();
    cur_notes[v_next] = note;
    unsigned ftw = midi_ftws[note];
    unsigned duty = 0;

    if (g_b_button & 0x01) {
        // constant duty cycle (lower on time for higher notes)
        duty = (velocity * velocity * g_volume) << 10;
    } else {
        // constant on time for low notes, limit max. duty cycle for high notes
        // volume controls on-time
        duty = (ftw >> 12) * velocity * g_volume / 128 * g_volume / 8;

        if (duty > (0x000FFFFF * MAX_DUTY_PERCENT / 100))
            duty = 0x000FFFFF * MAX_DUTY_PERCENT / 100;
        else if (duty < 0x00001000)  // lowest duty = .5 %
            duty = 0x00001000;
        duty <<= 12;
    }
    cur_duty[v_next] = duty;

    if (g_cur_bend != 0)
        ftw = get_bend_ftw(ftw, g_cur_bend);

    set_pulse(v_next, ftw, duty);

    print_voices();
}

static void pitch_bend(byte channel, int bend)
{
    // bend range = 2 cents = 0.5 ... 2.0
    g_cur_bend = bend;

    // bend all on notes
    for (unsigned v=0; v<N_VOICES; v++) {
        if (cur_notes[v] < 0 || cur_duty[v] < 0)
            continue;

        unsigned cur_ftw = midi_ftws[cur_notes[v]];
        set_pulse(v, get_bend_ftw(cur_ftw, bend), cur_duty[v]);
    }

    Serial.printf("B: %d\n", bend);
}

static void note_off(byte channel, byte note, byte velocity)
{
    if (note >= 128)
        return;

    for (unsigned v=0; v<N_VOICES; v++) {
        if (note == cur_notes[v]) {
            set_pulse(v, 0, 0);
            // Serial.printf("-[%d]: %d, %d\n", v, note, velocity);
            cur_notes[v] = -1;
            cur_duty[v] = -1;
            break;
        }
    }

    print_voices();
}

void midi_msg(const midi::Message<128u>& msg)
{
    Serial.printf("M: %x\n", msg.type);
}

void all_off()
{
    for (unsigned v=0; v<N_VOICES; v++) {
        cur_notes[v] = -1;
        cur_duty[v] = -1;
    }
    stop_pulse();
}

static void ctrl_change(byte channel, byte number, byte value)
{
    if (channel != 1)
        return;

    switch (number) {
        case 0:  // B. buttons
            g_b_button = value;
            break;
        // case 1:  // Modulation
        // case 2: // Breath
        case 7: // Volume
            // all_off();
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

static void midi_con(const ssrc_t & ssrc, const char* name)
{
    Serial.printf("midi_con(%x): %s\n", ssrc, name);
}

static void midi_discon(const ssrc_t & ssrc)
{
    Serial.printf("midi_discon(%x)\n", ssrc);
    all_off();
}

static void midi_except(const ssrc_t& ssrc, const Exception& e, const int32_t value)
{
    Serial.printf("midi_except(%x): %x %x\n", ssrc, e, value);
    if (e != ParticipantNotFoundException)
        all_off();
}

void init_musical()
{
    Serial.print("AppleMIDI device name: ");
    Serial.println(AppleMIDI.getName());

    // listen on midi channel 1 for events
    MIDI.begin();
    MIDI.setHandleNoteOn(note_on);
    MIDI.setHandleNoteOff(note_off);
    // MIDI.setHandleMessage(midi_msg);
    MIDI.setHandleControlChange(ctrl_change);
    MIDI.setHandlePitchBend(pitch_bend);
    AppleMIDI.setHandleConnected(midi_con);
    AppleMIDI.setHandleDisconnected(midi_discon);
    // AppleMIDI.setHandleException(midi_except);

    // Set up mDNS responder
    if (!MDNS.begin(AppleMIDI.getName()))
        Serial.println("Error setting up MDNS responder!");
    MDNS.addService("apple-midi", "udp", AppleMIDI.getPort());
    Serial.printf("Listening on UDP %d for rtp midi\n", AppleMIDI.getPort());
}


bool is_playing = false;
File midi_file = NULL;

typedef struct {
    uint16_t next_time;
    uint8_t next_cmd;
    uint8_t next_data_a;
    uint8_t next_data_b;
} t_midi_event;

t_midi_event next_evt;
uint32_t next_dt = 0;


static void get_next_event()
{
    // get next note
    int ret = midi_file.read(&next_evt, sizeof(midi_event));
    if (ret != sizeof(midi_event)) {
        is_playing = false;
        all_off();
        return;
    }
}


static void midi_player(bool reset)
{
    static uint32_t ms_ = 0;

    if (reset) {
        next_dt = 0;
        ms_ = millis();
    }

    if (!is_playing)
        return;

    uint32_t ms = millis();
    uint32_t dt = ms - ms_;

    if (next_dt == 0) {
        // next_dt = dt_from_file;
    }

    if (dt >= next_dt) {
        // play note
        // note_on()
        // note_off()
        // pitch_bend()
        // next_dt = ...
        ms_ = ms;
    }
}


void play_file(String fn)
{
    all_off();

    is_playing = false;
    if (midi_file)
        midi_file.close();

    midi_file = LittleFS.open(fn, "r");
    if (midi_file)
        is_playing = true;
    else
        Serial.printf("Failed to open %s", fn);

    midi_player(true);
}

void refresh_musical()
{
    MIDI.read();  // stream midi notes over UDP
    midi_player(false);  // play a simplified midi file
}
