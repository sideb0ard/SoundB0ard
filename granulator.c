#include <stdio.h>
#include <stdlib.h>

#include "granulator.h"
#include "mixer.h"

extern mixer *mixr;

granulator *new_granulator(int initial_num_grains, int initial_grain_len_ms)
{
    granulator *g = (granulator *)calloc(1, sizeof(granulator));
    g->num_grains_per_sec = initial_num_grains;
    g->grain_duration_ms = initial_grain_len_ms;
    g->wet_mix = 0.7;

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
    snprintf(status_string, MAX_PS_STRING_SZ,
             "numgrains: %d grainlen: %d fudge: %d wetmx: %f",
             g->num_grains_per_sec, g->grain_duration_ms, g->fudge_factor,
             g->wet_mix);
}

double granulator_process_audio(void *self, double input)
{
    granulator *g = (granulator *)self;
    delayline_write_delay_and_inc(&g->m_delay, input);

    int cur_grain = g->cloud_of_grains[g->cloud_read_idx];
    if (cur_grain != -99) {
        grain_activate(&g->m_grains[cur_grain]);
    }

    g->cloud_read_idx++;
    if (g->cloud_read_idx == g->cloud_read_end)
        g->cloud_read_idx = 0;

    double output = 0.;
    int samplesize_between_grains =
        g->cloud_read_end / g->num_grains_per_looplen;
    for (int i = 0; i < g->num_grains_per_looplen; i++) {
        int grain_idx = grain_generate_idx(&g->m_grains[i]);
        if (grain_idx != -1) {
            double env_amp = 1.;
            int twenty_percent_len = .2 * g->m_grains[i].grain_len_samples;
            int eighty_percent_len = .8 * g->m_grains[i].grain_len_samples;
            if (grain_idx < twenty_percent_len)
                env_amp = grain_idx / (double)twenty_percent_len;
            else if (grain_idx > eighty_percent_len)
                env_amp = (100 - grain_idx) / (double)twenty_percent_len;

            int modified_idx = (i + 1) * samplesize_between_grains * grain_idx;
            output += delayline_read_delay_at_idx(&g->m_delay, modified_idx) *
                      env_amp;
        }
    }

    return input * (1.0 - g->wet_mix) + output * g->wet_mix;
}

void granulator_refresh_cloud(granulator *g)
{
    g->cloud_read_idx = 0;
    g->cloud_read_end = mixr->loop_len_in_samples * 2;
    if (g->cloud_read_end > MAXCLOUD_LEN_SEC * SAMPLE_RATE) {
        printf(
            "Whoa nellie! my math must be terrible - this disnae work out!\n");
        printf("Increase size of MAXCLOUD_LEN_SEC\n");
        return;
    }

    double beat_time_in_secs = 60.0 / mixr->bpm;
    double seconds_in_cloud_loop = 8 * beat_time_in_secs;

    int num_grains_in_cloud_loop =
        seconds_in_cloud_loop * g->num_grains_per_sec;
    g->num_grains_per_looplen = num_grains_in_cloud_loop;
    //printf("BeattimeSecs: %f secs/cloudloop: %f num_grains_in_cloud_loop: %d\n",
    //       beat_time_in_secs, seconds_in_cloud_loop, num_grains_in_cloud_loop);

    for (int i = 0; i < num_grains_in_cloud_loop; i++) {
        int grain_dur = g->grain_duration_ms;
        if (g->fudge_factor != 0) {
            int fudgey = rand() % g->fudge_factor;
            int pos_or_neg = rand() % 2;
            if (pos_or_neg)
                grain_dur += fudgey;
            else
                grain_dur -= fudgey;
        }
        grain_initialize(&g->m_grains[i], grain_dur);
    }
    int spacing = g->cloud_read_end / num_grains_in_cloud_loop;
    printf("Spacing is %d (read-end: %d) / num_grains\n", spacing,
           g->cloud_read_end);
    int current_grain = 0;
    for (int i = 0; i < g->cloud_read_end; i++)
        g->cloud_of_grains[i] = -99;

    for (int i = 0; i < g->cloud_read_end; ) {
        g->cloud_of_grains[i] = current_grain++;
        int fudgey = spacing + (rand() % 20) * 44.1 ; // 20 msec in samples
        i += fudgey;
    }
}

void granulator_set_num_grains(granulator *g, int num_grains)
{
    g->num_grains_per_sec = num_grains;
    granulator_refresh_cloud(g);
}

void granulator_set_grain_len(granulator *g, int grain_len)
{
    g->grain_duration_ms = grain_len;
    granulator_refresh_cloud(g);
}

void granulator_set_fudge_factor(granulator *g, int fudge)
{
    g->fudge_factor = fudge;
    granulator_refresh_cloud(g);
}

void granulator_set_wet_mix(granulator *g, double mix)
{
    if (mix >= 0 && mix <= 1.0)
        g->wet_mix = mix;
    else
        printf("val must be between 0 and 1\n");
}

//////////////////////////////////////////////////////////////////////////////

void grain_initialize(grain *g, int grain_duration)
{
    g->grain_duration_ms = grain_duration;
    g->grain_len_samples = 44.1 * g->grain_duration_ms;
    g->audiobuffer_idx = 0;
}

void grain_activate(grain *g) { g->active = true; }

int grain_generate_idx(grain *g)
{
    if (!g->active)
        return -1;

    double my_idx = g->audiobuffer_idx;

    g->audiobuffer_idx++;
    if (g->audiobuffer_idx >= g->grain_len_samples) {
        g->audiobuffer_idx = 0;
        g->active = false;
    }

    return my_idx;
}
