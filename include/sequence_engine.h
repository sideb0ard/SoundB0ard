#pragma once

#include <stdbool.h>
#include <wchar.h>

#include "defjams.h"
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

enum
{
    ARP_UP,
    ARP_DOWN,
    ARP_UPDOWN,
    ARP_RAND,
    ARP_MAX_MODES,
};

enum
{
    FOLD_FWD,
    FOLD_BAK,
};

enum
{
    ARP_32,
    ARP_24,
    ARP_16,
    ARP_12,
    ARP_8,
    ARP_6,
    ARP_4,
    ARP_3,
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

typedef struct sequence_engine
{
    void *parent;
    unsigned int parent_type; // used for casting parent type

    int tick; // current 16th note tick from mixer

    // pattern state management
    midi_pattern patterns[MAX_NUM_MIDI_LOOPS];
    midi_event temporal_events[PPBAR];
    int pattern_multiloop_count[MAX_NUM_MIDI_LOOPS]; // how many times to play
                                                     // this loop
    midi_pattern backup_pattern_while_getting_crazy;

    int num_patterns;
    int cur_pattern;
    int cur_pattern_iteration;

    bool multi_pattern_mode;
    bool multi_pattern_loop_countdown_started;

    // end of pattern state management

    // these used to traverse the pattern in fun ways
    int count_by;
    int cur_step;

    int increment_by;
    int range_start;
    int range_len;
    int range_counter;

    bool fold;
    bool fold_direction;
    // end of pattern traversals

    // for sample and hold to downgrade sample_rate
    int sample_rate;
    double sample_rate_ratio;
    int sample_rate_counter;
    double cached_last_sample_left;
    double cached_last_sample_right;

    bool started; // used by drum seq
    bool allow_triplets;
    bool recording;
    bool single_note_mode; // ignore midi notes, just play midi_note_1
    bool chord_mode;
    int swing_setting;

    int transpose; // half step, i.e. midi num. Added to output value

    int midi_note_1;
    int midi_note_2;
    int midi_note_3;
    int octave;

    arpeggiator arp;

    bool restore_pending;

    int sustain_note_ms;

    uint16_t event_mask;
    bool enable_event_mask;
    int mask_mode; // 0 = MASK 1 = UNMASK
    int apply_mask_every_n;
    int event_mask_counter;

    bool follow_mixer_chord_changes;

    bool debug;

} sequence_engine;

void sequence_engine_init(sequence_engine *engine, void *parent,
                          unsigned int parent_synth_type);
void sequence_engine_reset(sequence_engine *engine);

bool sequence_engine_list_presets(unsigned int type);
void sequence_engine_set_sample_rate(sequence_engine *engine, int sample_rate);
void sequence_engine_status(sequence_engine *engine, wchar_t *status_string);
void sequence_engine_event_notify(void *self, broadcast_event event);
void sequence_engine_set_midi_note(sequence_engine *engine, int midi_note_idx,
                                   int root_key);

void sequence_engine_clear_pattern_ready_for_new_one(sequence_engine *engine,
                                                     int pattern_num);
void sequence_engine_set_multi_pattern_mode(sequence_engine *self,
                                            bool pattern_mode);
void sequence_engine_set_pattern_loop_num(sequence_engine *self,
                                          int pattern_num, int loop_num);

int sequence_engine_add_pattern(sequence_engine *self);
void sequence_engine_dupe_pattern(midi_pattern *from, midi_pattern *to);
void sequence_engine_switch_pattern(sequence_engine *self,
                                    unsigned int pattern_num);
// void sequence_engine_stop(sequence_engine *engine);
void sequence_engine_reset_pattern(sequence_engine *self,
                                   unsigned int pattern_num);
void sequence_engine_reset_pattern_all(sequence_engine *self);
void sequence_engine_reset_voices(sequence_engine *self);
void sequence_engine_pattern_to_string(sequence_engine *self, int pattern_num,
                                       wchar_t scratch[33]);
void sequence_engine_add_temporal_event(sequence_engine *self, int midi_tick,
                                        midi_event ev);
void sequence_engine_add_event(sequence_engine *self, int pattern_num,
                               int midi_tick, midi_event ev);
void sequence_engine_delete_event(sequence_engine *engine, int pat_num,
                                  int tick);

void sequence_engine_print_patterns(sequence_engine *engine);
void sequence_engine_nudge_pattern(sequence_engine *engine, int pattern_num,
                                   int sixteenth);
bool is_valid_pattern_num(sequence_engine *ns, int pattern_num);
void sequence_engine_import_midi_from_file(sequence_engine *engine,
                                           char *filename);
void sequence_engine_set_sustain_note_ms(sequence_engine *engine,
                                         int sustain_note_ms);
void sequence_engine_set_chord_mode(sequence_engine *engine, bool b);
void sequence_engine_set_single_note_mode(sequence_engine *engine, bool b);
void sequence_engine_set_backup_mode(sequence_engine *engine, bool b);
int sequence_engine_get_notes_from_pattern(midi_pattern loop,
                                           int return_midi_notes[10]);

int sequence_engine_change_octave_pattern(sequence_engine *engine,
                                          int pattern_num, int direction);
void sequence_engine_change_octave_midi_notes(sequence_engine *engine,
                                              unsigned int direction);
int sequence_engine_get_num_patterns(void *self);
void sequence_engine_set_num_patterns(void *self, int num_patterns);
int sequence_engine_get_num_notes(sequence_engine *engine);
void sequence_engine_make_active_track(void *self, int pattern_num);

void sequence_engine_add_note(sequence_engine *engine, int pattern_num,
                              int step, int midi_note, int amp, bool keep_note);
void sequence_engine_rm_note(sequence_engine *engine, int pattern_num,
                             int step);
void sequence_engine_mv_note(sequence_engine *engine, int pattern_num,
                             int fromstep, int tostep);
void sequence_engine_add_micro_note(sequence_engine *engine, int pattern_num,
                                    int step, int midi_note, int amp,
                                    bool keep_note);
void sequence_engine_rm_micro_note(sequence_engine *engine, int pattern_num,
                                   int step);
void sequence_engine_mv_micro_note(sequence_engine *engine, int pattern_num,
                                   int fromstep, int tostep);
midi_event *sequence_engine_get_pattern(void *self,
                                        int pattern_num);
void sequence_engine_set_pattern(void *self, int pattern_num,
                                 pattern_change_info change_info,
                                 midi_event *pattern);
void sequence_engine_set_pattern_to_riff(sequence_engine *engine);
void sequence_engine_set_pattern_to_current_key(sequence_engine *engine);

void sequence_engine_set_octave(sequence_engine *engine, int octave);
int sequence_engine_get_octave(sequence_engine *engine);

void sequence_engine_enable_arp(sequence_engine *engine, bool b);
void sequence_engine_set_arp_speed(sequence_engine *engine, unsigned int speed);
void sequence_engine_set_arp_mode(sequence_engine *engine, unsigned int mode);
void sequence_engine_do_arp(sequence_engine *engine, sound_generator *sg);
int arp_next_note(arpeggiator *arp);
void arp_add_last_note(arpeggiator *arp, int note);

void sequence_engine_set_follow_mixer_chords(sequence_engine *engine, bool b);
void sequence_engine_set_event_mask(sequence_engine *engine, uint16_t mask,
                                    int mask_every_n);
void sequence_engine_set_mask_every(sequence_engine *engine, int mask_every_n);
void sequence_engine_set_transpose(sequence_engine *engine, int transpose);
void sequence_engine_set_enable_event_mask(sequence_engine *engine, bool b);
bool sequence_engine_is_masked(sequence_engine *engine);
void sequence_engine_set_swing_setting(sequence_engine *engine,
                                       int swing_setting);
void sequence_engine_set_count_by(sequence_engine *engine, int count_by);
void sequence_engine_reset_step(sequence_engine *engine);
void sequence_engine_set_increment_by(sequence_engine *engine, int incr);
void sequence_engine_set_range_len(sequence_engine *engine, int range);
void sequence_engine_set_fold(sequence_engine *engine, bool b);
void sequence_engine_set_debug(sequence_engine *engine, bool b);
