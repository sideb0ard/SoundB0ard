#include <libgen.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "defjams.h"
#include "granulator.h"
#include "mixer.h"
#include "utils.h"

extern mixer *mixr;

granulator *new_granulator(char *filename)
{
    granulator *g = (granulator *)calloc(1, sizeof(granulator));
    g->vol = 0.7;
    g->active = true;
    g->started = false;

    g->granular_file_position = 0;
    g->granular_spray = 441; // 10ms * SR/1000;
    g->grain_duration_ms = 50;
    g->grains_per_sec = 30; // density
    g->grain_attack_time_pct = 2;
    g->grain_release_time_pct = 2;
    g->grain_selection = 0;

    g->sound_generator.gennext = &granulator_gennext;
    g->sound_generator.status = &granulator_status;
    g->sound_generator.getvol = &granulator_getvol;
    g->sound_generator.setvol = &granulator_setvol;
    g->sound_generator.start = &granulator_start;
    g->sound_generator.stop = &granulator_stop;
    g->sound_generator.get_num_tracks = &granulator_get_num_tracks;
    g->sound_generator.make_active_track = &granulator_make_active_track;
    g->sound_generator.type = GRANULATOR_TYPE;

    granulator_refresh_grain_stream(g);

    return g;
}

double granulator_gennext(void *self)
// void granulator_gennext(void* self, double* frame_vals, int framesPerBuffer)
{
    granulator *g = (granulator *)self;
    double val = 0;

    if (!g->active)
        return val;

    // int cur_loop_position = g->samples[l->cur_sample]->position;
    // int cur_grain = g->grain_stream[cur_loop_position];
    // if (cur_grain != -99) {
    //    // sound_grain_activate(&l->m_grains[cur_grain], true);
    //    l->m_cur_grain = cur_grain;
    //    sound_grain_reset(&l->m_grains[l->m_cur_grain]);
    //}

    // int grain_idx = sound_grain_generate_idx(&l->m_grains[l->m_cur_grain]);
    // int modified_idx =
    //    grain_idx % l->samples[l->cur_sample]->resampled_file_size;
    // val = l->samples[l->cur_sample]->resampled_file_bytes[modified_idx];

    val = effector(&g->sound_generator, val);
    val = envelopor(&g->sound_generator, val);

    return val * g->vol;
}

void granulator_status(void *self, wchar_t *status_string)
{
    granulator *g = (granulator *)self;
    swprintf(status_string, MAX_PS_STRING_SZ, WCOOL_COLOR_ORANGE
             "[GRANULATOR] Vol:%.2lf File:%s Len:%d"
             " grain_duration_ms:%d grains_per_sec:%d grain_spray_ms:%d\n",
             g->vol, "testfile", 256, g->grain_duration_ms, g->grains_per_sec,
             g->granular_spray);

    wcscat(status_string, WANSI_COLOR_RESET);
}

void granulator_start(void *self)
{
    granulator *g = (granulator *)self;
    g->active = true;
}

void granulator_stop(void *self)
{
    granulator *g = (granulator *)self;
    g->active = false;
    g->started = false;
}

double granulator_getvol(void *self)
{
    granulator *g = (granulator *)self;
    return g->vol;
}

void granulator_setvol(void *self, double v)
{
    granulator *g = (granulator *)self;
    if (v < 0.0 || v > 1.0) {
        return;
    }
    g->vol = v;
}

void granulator_del_self(granulator *g)
{
    // TODO delete file
    free(g);
}

void granulator_make_active_track(void *self, int track_num)
{
    // NOOP
}

int granulator_get_num_tracks(void *self) { return 1; }

void granulator_refresh_grain_stream(granulator *g)
{
    int looplen_in_seconds = mixr->loop_len_in_samples / (double)SAMPLE_RATE;
    g->num_grains_per_looplen = looplen_in_seconds * g->grains_per_sec;
    int grain_duration_samples = g->grain_duration_ms * 44.1;
    int spacing = mixr->loop_len_in_samples / g->num_grains_per_looplen;

    // create all the necessary grains with appropriate starting idx
    int grain_idx = g->granular_file_position;
    if (g->granular_spray > 0)
        grain_idx += rand() % g->granular_spray;
    int attack_time_pct = g->grain_attack_time_pct;
    int release_time_pct = g->grain_release_time_pct;
    for (int i = 0; i < g->num_grains_per_looplen; i++) {
        sound_grain_init(&g->m_grains[i], grain_duration_samples, grain_idx,
                         attack_time_pct, release_time_pct);
    }

    // reset, then populate the grain_stream positions with the
    // assocciated grain number
    for (int i = 0; i < mixr->loop_len_in_samples; i++)
        g->grain_stream[i] = -99;
    int current_grain = 0;
    for (int i = 0; i < mixr->loop_len_in_samples;) {
        // printf("adding to grain stream\n");
        g->grain_stream[i] = current_grain++;
        i += spacing;
    }
}

void granulator_set_granulate(granulator *l, bool b)
{
    if (b != 0 && b != 1) {
        printf("Must be true or false, yo!\n");
        return;
    }
    l->granulate_mode = b;
    granulator_refresh_grain_stream(l);
}
void granulator_set_grain_duration(granulator *l, int dur)
{
    // if (dur < MAX_GRAIN_DURATION) {
    l->grain_duration_ms = dur;
    granulator_refresh_grain_stream(l);
    //} else
    //    printf("Sorry, grain duration must be under %d\n",
    //    MAX_GRAIN_DURATION);
}

void granulator_set_grains_per_sec(granulator *l, int gps)
{
    l->grains_per_sec = gps;
    granulator_refresh_grain_stream(l);
}

void granulator_set_grain_selection_mode(granulator *l, unsigned int mode)
{
    l->grain_selection = mode;
    granulator_refresh_grain_stream(l);
}

void granulator_set_grain_attack_size_pct(granulator *l, int attack_pct)
{
    if (attack_pct < 50)
        l->grain_attack_time_pct = attack_pct;
    granulator_refresh_grain_stream(l);
}

void granulator_set_grain_release_size_pct(granulator *l, int release_pct)
{
    if (release_pct < 50)
        l->grain_release_time_pct = release_pct;
    granulator_refresh_grain_stream(l);
}

void granulator_set_granular_file_position(granulator *l, int pos)
{
    // int current_sample_file_size =
    //    l->samples[l->cur_sample]->resampled_file_size;
    // if (pos < current_sample_file_size)
    //    l->granular_file_position = pos;
    // else
    //    printf("Position must be less than file length:%d\n",
    //           current_sample_file_size);
    granulator_refresh_grain_stream(l);
}

void granulator_set_granular_spray(granulator *l, int spray_ms)
{
    int spray_samples = spray_ms * 44.1;
    l->granular_spray = spray_samples;
}

void sound_grain_init(sound_grain *g, int dur, int starting_idx, int attack_pct,
                      int release_pct)
{
    g->grain_len_samples = dur;
    g->audiobuffer_start_idx = starting_idx;
    g->audiobuffer_cur_pos = starting_idx;
    g->attack_time_pct = attack_pct;
    g->release_time_pct = release_pct;
}

void sound_grain_activate(sound_grain *g, bool b) { g->active = b; }

int sound_grain_generate_idx(sound_grain *g)
{
    // if (!g->active)
    //    return -1;

    double my_idx = g->audiobuffer_cur_pos;

    g->audiobuffer_cur_pos++;
    if (g->audiobuffer_cur_pos >=
        g->audiobuffer_start_idx + g->grain_len_samples) {
        g->audiobuffer_cur_pos = g->audiobuffer_start_idx;
        g->active = false;
    }
    return my_idx;
}

void sound_grain_reset(sound_grain *g)
{
    g->audiobuffer_cur_pos = g->audiobuffer_start_idx;
}

double sound_grain_env(sound_grain *g)
{
    double env_amp = 1.;
    double percent_pos = 100. / g->grain_len_samples *
                         (g->audiobuffer_cur_pos - g->audiobuffer_start_idx);

    if (percent_pos < g->attack_time_pct)
        env_amp *= percent_pos / g->attack_time_pct;
    else if (percent_pos > (100 - g->release_time_pct))
        env_amp *= (100 - percent_pos) / g->release_time_pct;

    return env_amp;
}
