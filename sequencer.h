#ifndef SEQUENCER_H
#define SEQUENCER_H

#include "filter_moogladder.h"
#include "sound_generator.h"
#include "stereodelay.h"
#include <sndfile.h>
#include <stdbool.h>
#include <wchar.h>

#define GRIDWIDTH (SEQUENCER_PATTERN_LEN / 4)
#define INTEGER_LENGTH pow(2, SEQUENCER_PATTERN_LEN)
#define NUM_SEQUENCER_PATTERNS 10
#define DEFAULT_AMP 0.7

typedef struct t_sample_pos {
    int position;
    int playing;
    int played;
} sample_pos;

typedef enum { MARKOVHAUS, MARKOVBOOMBAP } markovmodez;

typedef struct sequencer {
    SOUNDGEN sound_generator;
    sample_pos sample_positions[SEQUENCER_PATTERN_LEN];
    int matrix1[GRIDWIDTH][GRIDWIDTH];
    int matrix2[GRIDWIDTH][GRIDWIDTH];
    char *filename;
    int samplerate;
    int channels;

    int *buffer;
    int bufsize;
    int buf_num_channels;

    int patterns[NUM_SEQUENCER_PATTERNS];
    double pattern_position_amp[NUM_SEQUENCER_PATTERNS][SEQUENCER_PATTERN_LEN];
    int pattern_num_loops[NUM_SEQUENCER_PATTERNS];
    int backup_pattern_while_getting_crazy; // store current pattern so
                                            // algorithms can use slot
    int num_patterns;
    int cur_pattern;
    int cur_pattern_iteration;
    bool multi_pattern_mode;

    int tick;
    bool tickedyet;
    int swing;

    bool started; // to sync at top of loop

    bool game_of_life_on;
    int game_generation;

    bool markov_on;
    unsigned int markov_mode; // MARKOVHAUS or MARKOVBOOMBAP
    int markov_generation;

    int max_generation; // used for game of life or markov chain

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

} sequencer;

sequencer *new_seq(char *filename);
sequencer *new_seq_from_char_pattern(char *filename, char *pattern);
sequencer *new_seq_from_int_pattern(char *filename, int pattern);
sequencer *new_seq_from_char_array(char *filename, char **pattern, int nsteps);

void seq_status(void *self, wchar_t *ss);
void seq_setvol(void *self, double v);
void seq_set_sample_amp(sequencer *self, int pattern_num, int pattern_position,
                         double v);
void seq_set_sample_amp_from_char_pattern(sequencer *self, int pattern_num,
                                           char *amp_pattern);
void seq_set_random_sample_amp(sequencer *d, int pattern_num);
void add_char_pattern(sequencer *d, char *pattern);
void change_char_pattern(sequencer *d, int pattern_num, char *pattern);
void add_int_pattern(sequencer *d, int pattern);
void change_int_pattern(sequencer *d, int pattern_num, int pattern);
void swingrrr(void *self, int swing_setting);

void seq_set_multi_pattern_mode(sequencer *d, bool multi);
void seq_change_num_loops(sequencer *d, int pattern_num, int num_loops);

void int_pattern_to_array(int pattern, int *pat_array);
void pattern_char_to_int(char *chpattern, int *pattern);
void wchar_version_of_amp(sequencer *d, int pattern_num, wchar_t apattern[49]);

// game of life functionality
int seed_pattern(void);
void int_to_matrix(int pattern, int matrix[GRIDWIDTH][GRIDWIDTH]);
int matrix_to_int(int matrix[GRIDWIDTH][GRIDWIDTH]);
void next_life_generation(sequencer *d);
void next_markov_generation(sequencer *d);

int *load_file_to_buffer(char *filename, int *bufsize, SF_INFO *sf_info);

double seq_gennext(void *self);
double seq_getvol(void *self);

void seq_set_game_of_life(sequencer *d, bool on);
void seq_set_markov(sequencer *d, bool on);
void seq_set_markov_mode(sequencer *d, unsigned int mode);
void seq_set_backup_mode(sequencer *d, bool on);
void seq_set_max_generations(sequencer *d, int max);

void seq_parse_midi(sequencer *d, unsigned int data1, unsigned int data2);

#endif // SEQUENCER_H
