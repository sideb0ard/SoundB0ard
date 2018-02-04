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

typedef struct synthbase
{
    void *parent;
    unsigned int parent_synth_type;

    int tick; // current 16th note tick from mixer
    midi_events_loop melodies[MAX_NUM_MIDI_LOOPS];
    int melody_multiloop_count[MAX_NUM_MIDI_LOOPS];
    midi_events_loop backup_melody_while_getting_crazy;

    int sample_rate;
    double sample_rate_ratio;
    int sample_rate_counter;
    double cached_last_sample_left;
    double cached_last_sample_right;

    int num_melodies;
    int cur_melody;
    int cur_melody_iteration;

    bool multi_melody_mode;
    bool multi_melody_loop_countdown_started;

    bool recording;
    bool live_code_mode;

    bool morph_mode; // magical
    int morph_every_n_loops;
    int morph_generation;

    int last_midi_note;
    bool generate_mode; // magical
    int m_generate_src;
    int generate_every_n_loops;
    int generate_generation;

    int sustain_len_ms;

    int max_generation;

} synthbase;

void synthbase_init(synthbase *base, void *parent,
                    unsigned int parent_synth_type);

void synthbase_set_sample_rate(synthbase *base, int sample_rate);
void synthbase_status(synthbase *base, wchar_t *status_string);
void synthbase_event_notify(void *self, unsigned int event_type);

void synthbase_clear_melody_ready_for_new_one(synthbase *base, int melody_num);
void synthbase_generate_melody(synthbase *base);

void synthbase_set_multi_melody_mode(synthbase *self, bool melody_mode);
void synthbase_set_melody_loop_num(synthbase *self, int melody_num,
                                   int loop_num);

int synthbase_add_melody(synthbase *self);
void synthbase_dupe_melody(midi_events_loop *from, midi_events_loop *to);
void synthbase_switch_melody(synthbase *self, unsigned int melody_num);
void synthbase_stop(synthbase *base);
void synthbase_reset_melody(synthbase *self, unsigned int melody_num);
void synthbase_reset_melody_all(synthbase *self);
void synthbase_reset_voices(synthbase *self);
void synthbase_melody_to_string(synthbase *self, int melody_num,
                                wchar_t scratch[33]);
int synthbase_add_event(synthbase *self, int pattern_num, midi_event ev);
void synthbase_delete_event(synthbase *base, int pat_num, int tick);

void synthbase_copy_midi_loop(synthbase *self, int pattern_num,
                              midi_events_loop *target_loop);
void synthbase_replace_midi_loop(synthbase *base, midi_events_loop *source_loop,
                                 int melody_num);
void synthbase_print_melodies(synthbase *base);
void synthbase_nudge_melody(synthbase *base, int melody_num, int sixteenth);
bool is_valid_melody_num(synthbase *ns, int melody_num);
void synthbase_import_midi_from_file(synthbase *base, char *filename);
void synthbase_set_sustain_note_ms(synthbase *base, int sustain_note_ms);
void synthbase_set_generate_src(synthbase *base, int src);
void synthbase_set_generate_mode(synthbase *base, bool b);
void synthbase_set_morph_mode(synthbase *base, bool b);
void synthbase_set_backup_mode(synthbase *base, bool b);
void synthbase_morph(synthbase *base);
int synthbase_get_notes_from_melody(midi_events_loop *loop,
                                    int return_midi_notes[10]);

int synthbase_change_octave_melody(synthbase *base, int pattern_num,
                                   int direction);
int synthbase_get_num_tracks(void *self);
int synthbase_get_num_notes(synthbase *base);
void synthbase_make_active_track(void *self, int pattern_num);

void synthbase_add_note(synthbase *base, int pattern_num, int step,
                        int midi_note);
void synthbase_rm_note(synthbase *base, int pattern_num, int step);
void synthbase_mv_note(synthbase *base, int pattern_num, int fromstep,
                       int tostep);
void synthbase_add_micro_note(synthbase *base, int pattern_num, int step,
                              int midi_note);
void synthbase_rm_micro_note(synthbase *base, int pattern_num, int step);
void synthbase_mv_micro_note(synthbase *base, int pattern_num, int fromstep,
                             int tostep);
