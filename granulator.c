#include <stdlib.h>
#include <stdio.h>

#include "granulator.h"
#include "mixer.h"

extern mixer *mixr;

granulator *new_granulator(int initial_num_grains, int initial_grain_len_ms)
{
    granulator *g = (granulator *) calloc(1, sizeof(granulator));
    g->num_grains = initial_num_grains;
    g->grain_duration_ms = initial_grain_len_ms;

    delayline_init(&g->m_delay, 2.0 * SAMPLE_RATE);
    delayline_reset(&g->m_delay);

    granulator_refresh_cloud(g);

    g->m_fx.type = GRANULATOR;
    g->m_fx.enabled = true;
    g->m_fx.status = &granulator_status;
    g->m_fx.process = &granulator_process_audio;
    return g;
} 

void granulator_status(void *self, char *status_string)
{
    granulator *g = (granulator *)self;
    snprintf(status_string, MAX_PS_STRING_SZ, "numgrains: %d grainlen: %d",
             g->num_grains, g->grain_duration_ms);
}

double granulator_process_audio(void *self, double input)
{
    granulator *g = (granulator *)self;
    delayline_write_delay_and_inc(&g->m_delay, input);

    //if (g->cloud_of_grains[g->cloud_read_idx] == 1)
    //    printf("GRAIN!\n");

    g->cloud_read_idx++;
    if (g->cloud_read_idx == g->cloud_read_end)
        g->cloud_read_idx = 0;

    return input;
}

void granulator_refresh_cloud(granulator *g)
{
    g->cloud_read_idx = 0;
    g->cloud_read_end = mixr->loop_len_in_samples * 2;
    int spacing = g->cloud_read_end / g->num_grains;
    for (int i = 0; i < g->cloud_read_end; i++)
    {
        if (i % spacing == 0)
            g->cloud_of_grains[i] = 1;
        else
            g->cloud_of_grains[i] = 0;
    }
}
    
double ms_to_samples(int desired_ms)
{
    const int one_ms = 1 / 1000 * 44100;
    return desired_ms * one_ms;
}

void granulator_change_num_grains(granulator *g, int num_grains)
{
    g->num_grains = num_grains;
    granulator_refresh_cloud(g);
}

void granulator_change_grain_len(granulator *g, int grain_len)
{
    g->grain_duration_ms = grain_len;
    granulator_refresh_cloud(g);
}

void initialize_grain(grain *g, int grain_duration)
{
    g->grain_duration_ms = grain_duration;
    g->grain_len_samples = 44.1 * g->grain_duration_ms;
    g->audiobuffer_idx = 0;
}
