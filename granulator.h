#pragma once
#include <stdbool.h>

#include "defjams.h"
#include "delayline.h"
#include "fx.h"

#define MAXGRAINS 10000
#define MAXGRAIN_LEN_MS 500

#define MAXCLOUD_LEN_SEC 10

typedef struct grain {
    int grain_duration_ms;
    int grain_len_samples;
    int audiobuffer_idx;
    bool active;
} grain;

void grain_initialize(grain *g, int grain_duration_ms);
void grain_activate(grain *g);
int grain_generate_idx(grain *g);

typedef struct granulator {
    fx m_fx; // API

    delayline m_delay;

    grain m_grains[MAXGRAINS]; // actual grains
    int cloud_of_grains[MAXCLOUD_LEN_SEC * SAMPLE_RATE]; // position of grains
    int cloud_read_idx;
    int cloud_read_len;

    int grain_duration_ms;
    int num_grains_per_sec;
    int num_grains_per_looplen;

    int fudge_factor;

    double wet_mix;
} granulator;

granulator *new_granulator(int initial_num_grains, int initial_grain_len_ms);

void granulator_status(void *self, char *string);
double granulator_process_audio(void *self, double input);
double ms_to_samples(int desired_ms);
void granulator_refresh_cloud(granulator *g);
void granulator_set_num_grains(granulator *g, int num_grains);
void granulator_set_grain_len(granulator *g, int grain_len);
void granulator_set_fudge_factor(granulator *g, int fudge);
void granulator_set_wet_mix(granulator *g, double mix);
