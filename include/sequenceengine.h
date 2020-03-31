#pragma once

#include <stdbool.h>
#include <wchar.h>

#include <defjams.h>
#include <midimaaan.h>

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

enum
{
    FOLD_FWD,
    FOLD_BAK,
};

class sequenceengine
{
  public:
    sequenceengine();
    ~sequenceengine() = default;

  public:
    int tick; // current 16th note tick from mixer

    // pattern state management
    midi_pattern patterns[MAX_NUM_MIDI_LOOPS] = {};
    midi_event temporal_events[PPBAR] = {};
    int pattern_multiloop_count[MAX_NUM_MIDI_LOOPS] = {}; // how many times to
                                                          // play this loop
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

    bool restore_pending;

    int sustain_note_ms;

    uint16_t event_mask;
    bool enable_event_mask;
    int mask_mode; // 0 = MASK 1 = UNMASK
    int apply_mask_every_n;
    int event_mask_counter;

    bool follow_mixer_chord_changes;

    int pct_play; // percent probabilty note will play

    bool debug;
};

void sequence_engine_reset(sequenceengine *engine);

midi_event *sequence_engine_get_pattern(sequenceengine *engine,
                                        int pattern_num);
void sequence_engine_set_pattern(sequenceengine *engine, int pattern_num,
                                 pattern_change_info change_info,
                                 midi_event *pattern);
int sequence_engine_get_num_patterns(sequenceengine *engine);
void sequence_engine_set_num_patterns(sequenceengine *engine, int num_patterns);
void sequence_engine_make_active_pattern(sequenceengine *engine,
                                         int pattern_num);
bool sequence_engine_is_valid_pattern(sequenceengine *engine, int pattern_num);

////////////////////////////////////////////////////////////

bool sequence_engine_list_presets(unsigned int type);
void sequence_engine_set_sample_rate(sequenceengine *engine, int sample_rate);
void sequence_engine_status(sequenceengine *engine, wchar_t *status_string);
void sequence_engine_set_midi_note(sequenceengine *engine, int midi_note_idx,
                                   int root_key);

void sequence_engine_clear_pattern_ready_for_new_one(sequenceengine *engine,
                                                     int pattern_num);
void sequence_engine_set_multi_pattern_mode(sequenceengine *self,
                                            bool pattern_mode);
void sequence_engine_set_pattern_loop_num(sequenceengine *self, int pattern_num,
                                          int loop_num);

int sequence_engine_add_pattern(sequenceengine *self);
void sequence_engine_dupe_pattern(midi_pattern *from, midi_pattern *to);
void sequence_engine_switch_pattern(sequenceengine *self,
                                    unsigned int pattern_num);
// void sequence_engine_stop(sequenceengine *engine);
void sequence_engine_reset_pattern(sequenceengine *self,
                                   unsigned int pattern_num);
void sequence_engine_reset_pattern_all(sequenceengine *self);
void sequence_engine_reset_voices(sequenceengine *self);
void sequence_engine_pattern_to_string(sequenceengine *self, int pattern_num,
                                       wchar_t scratch[33]);
void sequence_engine_add_temporal_event(sequenceengine *self, int midi_tick,
                                        midi_event ev);
void sequence_engine_add_event(sequenceengine *self, int pattern_num,
                               int midi_tick, midi_event ev);
void sequence_engine_delete_event(sequenceengine *engine, int pat_num,
                                  int tick);

void sequence_engine_print_patterns(sequenceengine *engine);
void sequence_engine_nudge_pattern(sequenceengine *engine, int pattern_num,
                                   int sixteenth);
void sequence_engine_import_midi_from_file(sequenceengine *engine,
                                           char *filename);
void sequence_engine_set_sustain_note_ms(sequenceengine *engine,
                                         int sustain_note_ms);
void sequence_engine_set_chord_mode(sequenceengine *engine, bool b);
void sequence_engine_set_single_note_mode(sequenceengine *engine, bool b);
void sequence_engine_set_backup_mode(sequenceengine *engine, bool b);
int sequence_engine_get_notes_from_pattern(midi_pattern loop,
                                           int return_midi_notes[10]);

int sequence_engine_change_octave_pattern(sequenceengine *engine,
                                          int pattern_num, int direction);
void sequence_engine_change_octave_midi_notes(sequenceengine *engine,
                                              unsigned int direction);
int sequence_engine_get_num_notes(sequenceengine *engine);

void sequence_engine_add_note(sequenceengine *engine, int pattern_num, int step,
                              int midi_note, int amp, bool keep_note);
void sequence_engine_rm_note(sequenceengine *engine, int pattern_num, int step);
void sequence_engine_mv_note(sequenceengine *engine, int pattern_num,
                             int fromstep, int tostep);
void sequence_engine_add_micro_note(sequenceengine *engine, int pattern_num,
                                    int step, int midi_note, int amp,
                                    bool keep_note);
void sequence_engine_rm_micro_note(sequenceengine *engine, int pattern_num,
                                   int step);
void sequence_engine_mv_micro_note(sequenceengine *engine, int pattern_num,
                                   int fromstep, int tostep);
void sequence_engine_set_pattern_to_riff(sequenceengine *engine);
void sequence_engine_set_pattern_to_current_key(sequenceengine *engine);

void sequence_engine_set_octave(sequenceengine *engine, int octave);
int sequence_engine_get_octave(sequenceengine *engine);

void sequence_engine_set_follow_mixer_chords(sequenceengine *engine, bool b);
void sequence_engine_set_event_mask(sequenceengine *engine, uint16_t mask,
                                    int mask_every_n);
void sequence_engine_set_mask_every(sequenceengine *engine, int mask_every_n);
void sequence_engine_set_transpose(sequenceengine *engine, int transpose);
void sequence_engine_set_enable_event_mask(sequenceengine *engine, bool b);
bool sequence_engine_is_masked(sequenceengine *engine);
void sequence_engine_set_swing_setting(sequenceengine *engine,
                                       int swing_setting);
void sequence_engine_set_count_by(sequenceengine *engine, int count_by);
void sequence_engine_reset_step(sequenceengine *engine);
void sequence_engine_set_increment_by(sequenceengine *engine, int incr);
void sequence_engine_set_range_len(sequenceengine *engine, int range);
void sequence_engine_set_fold(sequenceengine *engine, bool b);
void sequence_engine_set_debug(sequenceengine *engine, bool b);
void sequence_engine_set_pct_play(sequenceengine *engine, int pct);

void sequence_engine_pattern_to_half_speed(sequenceengine *engine, int pat_num);
void sequence_engine_pattern_to_double_speed(sequenceengine *engine,
                                             int pat_num);
