#ifndef DRUM_H
#define DRUM_H

#include "sound_generator.h"
#include <sndfile.h>
#include <stdbool.h>
#include <wchar.h>

#define GRIDWIDTH (DRUM_PATTERN_LEN / 4)
#define INTEGER_LENGTH pow(2, DRUM_PATTERN_LEN)
#define NUM_DRUM_PATTERNS 10
#define DEFAULT_AMP 0.7

typedef struct t_sample_pos {
    int position;
    int playing;
    int played;
} sample_pos;

typedef enum { MARKOVHAUS, MARKOVBOOMBAP } markovmodez;

typedef struct t_drumr {
    SOUNDGEN sound_generator;
    sample_pos sample_positions[DRUM_PATTERN_LEN];
    int matrix1[GRIDWIDTH][GRIDWIDTH];
    int matrix2[GRIDWIDTH][GRIDWIDTH];
    char *filename;
    int samplerate;
    int channels;
    double vol;

    int *buffer;
    int bufsize;
    int buf_num_channels;

    int patterns[NUM_DRUM_PATTERNS];
    double pattern_position_amp[NUM_DRUM_PATTERNS][DRUM_PATTERN_LEN];
    int pattern_num_loops[NUM_DRUM_PATTERNS];
    int backup_pattern_while_getting_crazy; // store current pattern so
                                            // algorithms can use slot
    int num_patterns;
    int cur_pattern;
    int cur_pattern_iteration;
    bool multi_pattern_mode;

    int tick;
    bool tickedyet;
    int swing;
    int swing_setting;

    bool started; // to sync at top of loop

    bool game_of_life_on;
    int game_generation;

    bool markov_on;
    unsigned int markov_mode; // MARKOVHAUS or MARKOVBOOMBAP
    int markov_generation;

    int max_generation; // used for game of life or markov chain
} DRUM;

DRUM *new_drumr(char *filename);
DRUM *new_drumr_from_char_pattern(char *filename, char *pattern);
DRUM *new_drumr_from_int_pattern(char *filename, int pattern);
DRUM *new_drumr_from_char_array(char *filename, char **pattern, int nsteps);

// void drum_gennext(void* self, double* frame_vals, int framesPerBuffer);
void drum_status(void *self, wchar_t *ss);
void drum_setvol(void *self, double v);
void drum_set_sample_amp(DRUM *self, int pattern_num, int pattern_position,
                         double v);
void drum_set_sample_amp_from_char_pattern(DRUM *self, int pattern_num,
                                           char *amp_pattern);
void drum_set_random_sample_amp(DRUM *d, int pattern_num);
// void update_pattern(void *self, int newpattern);
void add_char_pattern(DRUM *d, char *pattern);
void change_char_pattern(DRUM *d, int pattern_num, char *pattern);
void add_int_pattern(DRUM *d, int pattern);
void change_int_pattern(DRUM *d, int pattern_num, int pattern);
void swingrrr(void *self, int swing_setting);

void drumr_set_multi_pattern_mode(DRUM *d, bool multi);
void drumr_change_num_loops(DRUM *d, int pattern_num, int num_loops);

void int_pattern_to_array(int pattern, int *pat_array);
void pattern_char_to_int(char *chpattern, int *pattern);
void wchar_version_of_amp(DRUM *d, int pattern_num, wchar_t apattern[49]);

// game of life functionality
int seed_pattern(void);
void int_to_matrix(int pattern, int matrix[GRIDWIDTH][GRIDWIDTH]);
int matrix_to_int(int matrix[GRIDWIDTH][GRIDWIDTH]);
void next_life_generation(DRUM *d);
void next_markov_generation(DRUM *d);

int *load_file_to_buffer(char *filename, int *bufsize, SF_INFO *sf_info);

double drum_gennext(void *self);
double drum_getvol(void *self);

void seq_set_game_of_life(DRUM *d, bool on);
void seq_set_markov(DRUM *d, bool on);
void seq_set_markov_mode(DRUM *d, unsigned int mode);
void seq_set_backup_mode(DRUM *d, bool on);
void seq_set_max_generations(DRUM *d, int max);

#endif // DRUM_H
