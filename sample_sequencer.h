#pragma once

#include "filter_moogladder.h"
#include "sequencer.h"
#include "sound_generator.h"
#include "stereodelay.h"

#include <sndfile.h>
#include <stdbool.h>
#include <wchar.h>

#define DEFAULT_AMP 0.7
#define MAX_CONCURRENT_SAMPLES 10 // arbitrary

typedef struct t_sample_pos
{
    int position;
    int playing;
    int played;
} sample_pos;

typedef struct sample_sequencer
{
    soundgenerator sound_generator;
    sequencer m_seq;
    sample_pos sample_positions[PPBAR];
    int samples_now_playing[MAX_CONCURRENT_SAMPLES]; // contains midi tick of
                                                     // current samples
    // rathern than walking array -1 means not playing
    char filename[1024];
    int samplerate;
    int channels;

    double *buffer;
    int bufsize;
    // int buf_num_channels;

    int swing;

    bool started; // to sync at top of loop

    double vol;

    filter_moogladder m_filter;
    double m_fc_control;
    double m_q_control;
    stereodelay m_delay_fx;

    double m_delay_time_msec;
    double m_feedback_pct;
    double m_delay_ratio;
    double m_wet_mix;
    unsigned int m_delay_mode;

} sample_sequencer;

sample_sequencer *new_sample_seq(char *filename);
sample_sequencer *new_sample_seq_from_char_pattern(char *filename,
                                                   char *pattern);
sample_sequencer *new_sample_seq_from_int_pattern(char *filename, int pattern);
sample_sequencer *new_sample_seq_from_char_array(char *filename, char **pattern,
                                                 int nsteps);

int get_a_sample_seq_position(sample_sequencer *ss);
int sample_seq_get_num_tracks(void *s);
void sample_seq_make_active_track(void *s, int track_num);

void sampleseq_del_self(void *self);
void sample_seq_status(void *self, wchar_t *ss);
void sample_seq_setvol(void *self, double v);
void sample_start(void *self);
void sample_stop(void *self);
double sample_seq_gennext(void *self);
double sample_seq_getvol(void *self);

void sample_seq_import_file(sample_sequencer *s, char *filename);
void sample_seq_parse_midi(sample_sequencer *s, unsigned int data1,
                           unsigned int data2);

void sample_sequencer_reset_samples(sample_sequencer *seq);
