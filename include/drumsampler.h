#pragma once

#include "envelope_generator.h"
#include "filter_moogladder.h"
#include "sequence_engine.h"
#include "sound_generator.h"
#include "stereodelay.h"

#include <sndfile.h>
#include <stdbool.h>
#include <wchar.h>

#define DEFAULT_AMP 0.7
#define MAX_CONCURRENT_SAMPLES 10 // arbitrary

typedef struct sample_pos
{
    int position;
    int playing;
    int played;
    double audiobuffer_cur_pos;
    double audiobuffer_inc;
    double audiobuffer_pitch;
    double amp;
    double speed;
    double start_pos_pct;
    double end_pos_pct;
} sample_pos;

typedef struct drumsampler
{
    soundgenerator sound_generator;
    sequence_engine engine;

    bool glitch_mode;
    int glitch_rand_factor;

    sample_pos sample_positions[PPBAR];
    int samples_now_playing[MAX_CONCURRENT_SAMPLES]; // contains midi tick of
                                                     // current samples
    int velocity_now_playing[MAX_CONCURRENT_SAMPLES];

    char filename[1024];
    int samplerate;
    int channels;

    envelope_generator eg;
    bool envelope_enabled;

    double *buffer;
    int bufsize;
    int buf_end_pos; // this will always be shorter than bufsize for cutting off
                     // sample earlier
    double buffer_pitch;
    // int buf_num_channels;

    int swing;
    bool started; // to sync at top of loop

    double vol;

} drumsampler;

drumsampler *new_drumsampler(char *filename);

int get_a_drumsampler_position(drumsampler *ss);
int drumsampler_get_num_patterns(void *s);
void drumsampler_set_num_patterns(void *s, int num_patterns);
void drumsampler_make_active_track(void *s, int track_num);
bool drumsampler_is_valid_pattern(void *self, int pattern_num);

void drumsampler_del_self(void *self);
void drumsampler_status(void *self, wchar_t *status_string);
void drumsampler_setvol(void *self, double v);
double drumsampler_getvol(void *self);
void drumsampler_start(void *self);
void drumsampler_stop(void *self);
stereo_val drumsampler_gennext(void *self);

midi_event *drumsampler_get_pattern(void *self, int pattern_num);
void drumsampler_set_pattern(void *self, int pattern_num,
                             pattern_change_info change_info,
                             midi_event *pattern);

void drumsampler_import_file(drumsampler *s, char *filename);
void drumsampler_reset_samples(drumsampler *seq);
void drumsampler_set_pitch(drumsampler *seq, double v);
void drumsampler_set_cutoff_percent(drumsampler *seq, unsigned int percent);
void drumsampler_note_on(drumsampler *ds);
void drumsampler_enable_envelope_generator(drumsampler *ds, bool b);
void drumsampler_set_attack_time(drumsampler *ds, double val);
void drumsampler_set_decay_time(drumsampler *ds, double val);
void drumsampler_set_sustain_lvl(drumsampler *ds, double val);
void drumsampler_set_release_time(drumsampler *ds, double val);
void drumsampler_set_glitch_mode(drumsampler *ds, bool b);
void drumsampler_set_glitch_rand_factor(drumsampler *ds, int pct);
