#ifndef DRUM_H
#define DRUM_H

#include "sound_generator.h"
#include <sndfile.h>
#include <stdbool.h>

#define GRIDWIDTH (DRUM_PATTERN_LEN / 4)
#define INTEGER_LENGTH pow(2, DRUM_PATTERN_LEN)

typedef struct t_sample_pos {
    int position;
    int playing;
    int played;
} sample_pos;

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

    int patterns[10];
    int num_patterns;
    int cur_pattern_num;

    int tick;
    int swing;
    int swing_setting;

    bool game_of_life_on;
    int game_generation;
} DRUM;

DRUM *new_drumr(char *filename);
DRUM *new_drumr_from_char_pattern(char *filename, char *pattern);
DRUM *new_drumr_from_int_pattern(char *filename, int pattern);

// void drum_gennext(void* self, double* frame_vals, int framesPerBuffer);
void drum_status(void *self, char *ss);
void drum_setvol(void *self, double v);
// void update_pattern(void *self, int newpattern);
void add_pattern(void *self, char *pattern);
void swingrrr(void *self, int swing_setting);

void int_pattern_to_array(int pattern, int *pat_array);
void pattern_char_to_int(char *chpattern, int *pattern);

// game of life functionality
int seed_pattern(void);
void int_to_matrix(int pattern, int matrix[GRIDWIDTH][GRIDWIDTH]);
int matrix_to_int(int matrix[GRIDWIDTH][GRIDWIDTH]);
void next_life_generation(void *d);

int *load_file_to_buffer(char *filename, int *bufsize, SF_INFO *sf_info);

double drum_gennext(void *self);
double drum_getvol(void *self);

#endif // DRUM_H
