#pragma once

#include <stdbool.h>
#include <wchar.h>

#include "midimaaan.h"

#define MAX_NUM_MIDI_LOOPS 64
#define MAX_VOICES 3

#define MIN_DETUNE_CENTS -50.0
#define MAX_DETUNE_CENTS 50.0
#define DEFAULT_DETUNE_CENTS 0.0

#define MIN_PULSE_WIDTH_PCT 1.0
#define MAX_PULSE_WIDTH_PCT 99.0
#define DEFAULT_PULSE_WIDTH_PCT 50.0
#define MIN_NOISE_OSC_AMP_DB -96.0
#define MAX_NOISE_OSC_AMP_DB 0.0
#define DEFAULT_NOISE_OSC_AMP_DB -96.0
#define MIN_SUB_OSC_AMP_DB -96.0
#define MAX_SUB_OSC_AMP_DB 0.0
#define DEFAULT_SUB_OSC_AMP_DB -96.0

#define DEFAULT_LEGATO_MODE 0
#define DEFAULT_RESET_TO_ZERO 0
#define DEFAULT_FILTER_KEYTRACK 0
#define DEFAULT_FILTER_KEYTRACK_INTENSITY 0.5
#define DEFAULT_VELOCITY_TO_ATTACK 0
#define DEFAULT_NOTE_TO_DECAY 0
#define DEFAULT_MIDI_PITCHBEND 0
#define DEFAULT_MIDI_MODWHEEL 0
#define DEFAULT_MIDI_VOLUME 127
#define DEFAULT_MIDI_PAN 64
#define DEFAULT_MIDI_EXPRESSION 0
#define DEFAULT_PORTAMENTO_TIME_MSEC 0.0

#define MAX_NOTES_ARP 3

enum {
    ARP_UP,
    ARP_DOWN,
    ARP_UPDOWN,
    ARP_RAND,
    ARP_MAX_MODES,
};

enum {
    ARP_32,
    ARP_16,
    ARP_8,
    ARP_4,
    ARP_MAX_SPEEDS,
};


typedef struct arpeggiator
{
    bool enable;
    unsigned int mode;
    unsigned int speed;
    unsigned int direction; // track UP or DOWN for arp mode UPDOWN
    int last_midi_notes[MAX_NOTES_ARP];
    int last_midi_notes_idx;
} arpeggiator;

typedef struct synthbase
{
    void *parent;
    unsigned int parent_synth_type;

    int tick; // current 16th note tick from mixer
    midi_pattern patterns[MAX_NUM_MIDI_LOOPS];
    int pattern_multiloop_count[MAX_NUM_MIDI_LOOPS];
    midi_pattern backup_pattern_while_getting_crazy;

    int sample_rate;
    double sample_rate_ratio;
    int sample_rate_counter;
    double cached_last_sample_left;
    double cached_last_sample_right;

    int num_patterns;
    int cur_pattern;
    int cur_pattern_iteration;

    bool multi_pattern_mode;
    bool multi_pattern_loop_countdown_started;

    bool recording;
    bool note_mode;
    bool chord_mode;

    int midi_note;
    int octave;

    arpeggiator arp;
    // arpeggiator m_arp;

    // bool generate_mode; // magical
    // int generate_src;
    // int generate_every_n_loops;
    // int generate_generation;

    bool restore_pending;

    int sustain_note_ms;

    int max_generation;

} synthbase;

static const char MOOG_PRESET_FILENAME[] = "settings/moogpresets.dat";
static const char DX_PRESET_FILENAME[] = "settings/dxpresets.dat";

void synthbase_init(synthbase *base, void *parent,
                    unsigned int parent_synth_type);

bool synthbase_list_presets(unsigned int type);
void synthbase_set_sample_rate(synthbase *base, int sample_rate);
void synthbase_status(synthbase *base, wchar_t *status_string);
void synthbase_event_notify(void *self, unsigned int event_type);
void synthbase_set_rand_key(synthbase *base);
void synthbase_set_midi_note(synthbase *base, int root_key);

void synthbase_clear_pattern_ready_for_new_one(synthbase *base,
                                               int pattern_num);
void synthbase_generate_pattern(synthbase *base, int gen_src, bool keep_notes,
                                bool restore_pattern);

void synthbase_set_multi_pattern_mode(synthbase *self, bool pattern_mode);
void synthbase_set_pattern_loop_num(synthbase *self, int pattern_num,
                                    int loop_num);

int synthbase_add_pattern(synthbase *self);
void synthbase_dupe_pattern(midi_pattern *from, midi_pattern *to);
void synthbase_switch_pattern(synthbase *self, unsigned int pattern_num);
void synthbase_stop(synthbase *base);
void synthbase_reset_pattern(synthbase *self, unsigned int pattern_num);
void synthbase_reset_pattern_all(synthbase *self);
void synthbase_reset_voices(synthbase *self);
void synthbase_pattern_to_string(synthbase *self, int pattern_num,
                                 wchar_t scratch[33]);
void synthbase_add_event(synthbase *self, int pattern_num, int midi_tick,
                         midi_event ev);
void synthbase_delete_event(synthbase *base, int pat_num, int tick);

void synthbase_print_patterns(synthbase *base);
void synthbase_nudge_pattern(synthbase *base, int pattern_num, int sixteenth);
bool is_valid_pattern_num(synthbase *ns, int pattern_num);
void synthbase_import_midi_from_file(synthbase *base, char *filename);
void synthbase_set_sustain_note_ms(synthbase *base, int sustain_note_ms);
void synthbase_set_generate_src(synthbase *base, int src);
void synthbase_set_chord_mode(synthbase *base, bool b);
void synthbase_set_note_mode(synthbase *base, bool b);
void synthbase_set_morph_mode(synthbase *base, bool b);
void synthbase_set_backup_mode(synthbase *base, bool b);
void synthbase_morph(synthbase *base);
int synthbase_get_notes_from_pattern(midi_pattern loop,
                                     int return_midi_notes[10]);

int synthbase_change_octave_pattern(synthbase *base, int pattern_num,
                                    int direction);
int synthbase_get_num_patterns(void *self);
void synthbase_set_num_patterns(void *self, int num_patterns);
int synthbase_get_num_notes(synthbase *base);
void synthbase_make_active_track(void *self, int pattern_num);

void synthbase_add_note(synthbase *base, int pattern_num, int step,
                        int midi_note, bool keep_note);
void synthbase_rm_note(synthbase *base, int pattern_num, int step);
void synthbase_mv_note(synthbase *base, int pattern_num, int fromstep,
                       int tostep);
void synthbase_add_micro_note(synthbase *base, int pattern_num, int step,
                              int midi_note, bool keep_note);
void synthbase_rm_micro_note(synthbase *base, int pattern_num, int step);
void synthbase_mv_micro_note(synthbase *base, int pattern_num, int fromstep,
                             int tostep);
midi_event *synthbase_get_pattern(synthbase *base, int pattern_num);
void synthbase_set_pattern(void *self, int pattern_num, midi_event *pattern);

void synthbase_set_octave(synthbase *base, int octave);
int synthbase_get_octave(synthbase *base);


void synthbase_enable_arp(synthbase *base, bool b);
void synthbase_set_arp_speed(synthbase *base, unsigned int speed);
void synthbase_set_arp_mode(synthbase *base, unsigned int mode);
int arp_next_note(arpeggiator *arp);
void arp_add_last_note(arpeggiator *arp, int note);
