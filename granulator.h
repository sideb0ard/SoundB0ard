#ifndef GRANULATOR_H
#define GRANULATOR_H

#include "filter_moogladder.h"
#include "sound_generator.h"
#include "stereodelay.h"
#include <stdbool.h>
#include <wchar.h>

#define MAX_SAMPLES_PER_LOOPER 10
#define MAX_SOUND_GRAINS 1000
#define MAX_GRAIN_STREAM_LEN_SEC 10

typedef struct sound_grain {
    int grain_len_samples;
    int audiobuffer_num;
    int audiobuffer_start_idx;
    int audiobuffer_cur_pos;
    int release_time_pct; // percent of grain_len_samples
    int attack_time_pct;  // percent of grain_len_samples
    bool active;
} sound_grain;

enum {
    GRAIN_SEQUENTIAL,
    GRAIN_RANDOM,
    GRAIN_REVERSED,
    GRAIN_NUM_SELECTION_MODES
};

typedef struct granulator {
    SOUNDGEN sound_generator;

    bool active;
    bool started;

    bool granulate_mode;
    int granular_file_position;
    int granular_spray;
    int grain_duration_ms;
    int grains_per_sec;
    int num_grains_per_looplen;
    unsigned int grain_selection;
    sound_grain m_grains[MAX_SOUND_GRAINS];
    int m_cur_grain;
    int grain_stream[MAX_GRAIN_STREAM_LEN_SEC * SAMPLE_RATE];
    int grain_stream_len_samples;
    int grain_attack_time_pct;
    int grain_release_time_pct;

    double vol;
} granulator;

granulator *new_granulator(char *filename);

// void granulator_gennext(void* self, double* frame_vals, int framesPerBuffer);
double granulator_gennext(void *self);

void granulator_status(void *self, wchar_t *ss);
void granulator_setvol(void *self, double v);
double granulator_getvol(void *self);
void granulator_start(void *self);
void granulator_stop(void *self);

void granulator_refresh_grain_stream(granulator *g);
void granulator_set_granulate(granulator *g, bool b);
void granulator_set_grain_duration(granulator *g, int dur);
void granulator_set_grains_per_sec(granulator *g, int gps);
void granulator_set_grain_attack_size_pct(granulator *g, int att);
void granulator_set_grain_release_size_pct(granulator *g, int rel);
void granulator_set_grain_selection_mode(granulator *g, unsigned int mode);
void granulator_set_granular_file_position(granulator *g, int position);
void granulator_set_granular_spray(granulator *g, int spray_ms);

void sound_grain_init(sound_grain *g, int dur, int starting_idx, int attack_pct,
                      int release_pct);
void sound_grain_activate(sound_grain *g, bool b);
int sound_grain_generate_idx(sound_grain *g);
double sound_grain_env(sound_grain *g);
void sound_grain_reset(sound_grain *g);

void granulator_del_self(granulator *g);

#endif // GRANULATOR
