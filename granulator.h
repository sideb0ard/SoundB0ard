#pragma once
#include <stdbool.h>

#include "defjams.h"
#include "delayline.h"
#include "fx.h"

#define MAXGRAINS 1000
#define MAXGRAIN_LEN_MS 500

#define MAXCLOUD_LEN_SEC 10

typedef struct grain
{
    int grain_duration_ms;
    int grain_len_samples;
    int audiobuffer_idx;
} grain;

void initialize_grain(grain *g, int grain_len);

typedef struct granulator
{
    fx m_fx; // API

    delayline m_delay;

    grain m_grains[MAXGRAINS];
    int cloud_of_grains[MAXCLOUD_LEN_SEC * SAMPLE_RATE]; // value is index of m_grains
    int cloud_read_idx;
    int cloud_read_end;

    int grain_duration_ms;
    int num_grains;

    double mix;
} granulator;

granulator *new_granulator(int initial_num_grains, int initial_grain_len_ms);

void granulator_status(void *self, char *string);
double granulator_process_audio(void *self, double input);
double ms_to_samples(int desired_ms);
void granulator_refresh_cloud(granulator *g);
void granulator_change_num_grains(granulator *g, int num_grains);
void granulator_change_grain_len(granulator *g, int grain_len);
