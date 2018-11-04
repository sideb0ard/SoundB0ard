#pragma once

#include "filter_moogladder.h"
#include "sound_generator.h"
#include "step_sequencer.h"
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
    step_sequencer m_seq;

    sample_pos sample_positions[PPBAR];
    int samples_now_playing[MAX_CONCURRENT_SAMPLES]; // contains midi tick of
                                                     // current samples
    int velocity_now_playing[MAX_CONCURRENT_SAMPLES];

    // rathern than walking array -1 means not playing
    char filename[1024];
    int samplerate;
    int channels;

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
drumsampler *new_drumsampler_from_char_pattern(char *filename,
                                                   char *pattern);
drumsampler *new_drumsampler_from_int_pattern(char *filename, int pattern);
drumsampler *new_drumsampler_from_char_array(char *filename, char **pattern,
                                                 int nsteps);

int get_a_drumsampler_position(drumsampler *ss);
int drumsampler_get_num_patterns(void *s);
void drumsampler_set_num_patterns(void *s, int num_patterns);
void drumsampler_make_active_track(void *s, int track_num);
void drumsampler_event_notify(void *s, unsigned int event_type);
bool drumsampler_is_valid_pattern(void *self, int pattern_num);

void drumsampler_del_self(void *self);
void drumsampler_status(void *self, wchar_t *status_string);

void drumsampler_setvol(void *self, double v);
double drumsampler_getvol(void *self);

void sample_start(void *self);
void sample_stop(void *self);

stereo_val drumsampler_gennext(void *self);

midi_event *drumsampler_get_pattern(void *self, int pattern_num);
void drumsampler_set_pattern(void *self, int pattern_num, midi_event *pattern);

void drumsampler_import_file(drumsampler *s, char *filename);
void drumsampler_reset_samples(drumsampler *seq);
void drumsampler_set_pitch(drumsampler *seq, double v);
void drumsampler_set_cutoff_percent(drumsampler *seq,
                                         unsigned int percent);
