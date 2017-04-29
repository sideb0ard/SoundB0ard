#pragma once

#include "filter_moogladder.h"
#include "sequencer.h"
#include "sound_generator.h"
#include "stereodelay.h"

#include <sndfile.h>
#include <stdbool.h>
#include <wchar.h>

#define DEFAULT_AMP 0.7

typedef struct t_sample_pos {
    int position;
    int playing;
    int played;
} sample_pos;

typedef struct sample_sequencer {
    SOUNDGEN sound_generator;
    sequencer m_seq;
    sample_pos sample_positions[PPBAR];
    char *filename;
    int samplerate;
    int channels;

    int *buffer;
    int bufsize;
    int buf_num_channels;

    int swing;

    bool started; // to sync at top of loop

    stereodelay m_delay_fx;
    // midi - top row
    double m_fc_control;
    double m_q_control;
    int swing_setting;
    double vol;
    filter_moogladder m_filter;
    // midi - bottom row, mode 1
    double m_delay_time_msec;
    double m_feedback_pct;
    double m_delay_ratio;
    double m_wet_mix;
    unsigned int m_delay_mode; // pad5 button

} sample_sequencer;

sample_sequencer *new_sample_seq(char *filename);
sample_sequencer *new_sample_seq_from_char_pattern(char *filename,
                                                   char *pattern);
sample_sequencer *new_sample_seq_from_int_pattern(char *filename, int pattern);
sample_sequencer *new_sample_seq_from_char_array(char *filename, char **pattern,
                                                 int nsteps);

int sample_seq_get_num_tracks(void *s);

void sample_seq_del(sample_sequencer *s);
void sample_seq_status(void *self, wchar_t *ss);
void sample_seq_setvol(void *self, double v);
double sample_seq_gennext(void *self);
double sample_seq_getvol(void *self);

void swingrrr(void *self, int swing_setting);

int *load_file_to_buffer(char *filename, int *bufsize, SF_INFO *sf_info);
void sample_seq_parse_midi(sample_sequencer *s, unsigned int data1,
                           unsigned int data2);
