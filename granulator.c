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
    g->granulate_mode = false;
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

    granulator_refresh_grain_stream(l);

    return g;
}

double granulator_gennext(void *self)
// void granulator_gennext(void* self, double* frame_vals, int framesPerBuffer)
{
    granulator *g = (granulator *)self;
    double val = 0;

    if (!g->active)
        return val;

    int cur_loop_position = g->samples[l->cur_sample]->position;
    int cur_grain = g->grain_stream[cur_loop_position];
    if (cur_grain != -99) {
        // sound_grain_activate(&l->m_grains[cur_grain], true);
        l->m_cur_grain = cur_grain;
        sound_grain_reset(&l->m_grains[l->m_cur_grain]);
    }

    int grain_idx = sound_grain_generate_idx(&l->m_grains[l->m_cur_grain]);
    int modified_idx =
        grain_idx % l->samples[l->cur_sample]->resampled_file_size;
    val = l->samples[l->cur_sample]->resampled_file_bytes[modified_idx];

    val = effector(&l->sound_generator, val);
    val = envelopor(&l->sound_generator, val);

    return val * l->vol;
}

void granulator_status(void *self, wchar_t *status_string)
{
    granulator *l = (granulator *)self;
    swprintf(
        status_string, MAX_PS_STRING_SZ, WCOOL_COLOR_GREEN
        "[granulator] Vol:%.2lf MultiMode:%s CurSample:%d Len:%d Granulate:%s"
        " grain_duration_ms:%d grains_per_sec:%d grain_spray_ms:%d\n"
        "      ScramblrrrMode:%s ScrambleGen:%d ScrambleEveryN:%d "
        "StutterMode:%s StutterGen:%d StutterEveryN:%d\n"
        "      MaxGen:%d  Active:%s Position:%d SampleLength:%d",
        l->vol, l->multi_sample_mode ? "true" : "false", l->cur_sample,
        l->samples[l->cur_sample]->resampled_file_size,
        l->granulate_mode ? "true" : "false", l->grain_duration_ms,
        l->grains_per_sec, l->granular_spray,
        l->scramblrrr_mode ? "true" : "false", l->scramble_generation,
        l->scramble_every_n_loops, l->stutter_mode ? "true" : "false",
        l->stutter_generation, l->stutter_every_n_loops, l->max_generation,
        l->active ? " true" : "false", l->samples[l->cur_sample]->position,
        l->samples[l->cur_sample]->resampled_file_size);

    int strlen_left = MAX_PS_STRING_SZ - wcslen(status_string);
    wchar_t granulator_details[strlen_left];
    for (int i = 0; i < l->num_samples; i++) {
        swprintf(granulator_details, 128,
                 L"\n      [" WANSI_COLOR_WHITE "%d" WCOOL_COLOR_GREEN "]"
                 " %s - looplen: %d numloops: %d",
                 i, basename(l->samples[i]->filename), l->samples[i]->loop_len,
                 l->sample_num_loops[i]);
        wcslcat(status_string, granulator_details, strlen_left);
    }
    wcscat(status_string, WANSI_COLOR_RESET);
}

void granulator_start(void *self)
{
    granulator *l = (granulator *)self;
    l->active = true;
    // l->started = false;
    // l->samples[l->cur_sample]->position = 0;
    // l->scramblrrr->position = 0;
}

void granulator_stop(void *self)
{
    granulator *l = (granulator *)self;
    l->active = false;
    l->started = false;
    l->samples[l->cur_sample]->position = 0;
    l->scramblrrr->position = 0;
}

double granulator_getvol(void *self)
{
    granulator *l = (granulator *)self;
    return l->vol;
}

void granulator_setvol(void *self, double v)
{
    granulator *l = (granulator *)self;
    if (v < 0.0 || v > 1.0) {
        return;
    }
    l->vol = v;
}

void granulator_del_self(granulator *s)
{
    for (int i = 0; i < s->num_samples; i++) {
        printf("Dleeeting samples\n");
        file_sample_free(s->samples[i]);
    }
    free(s);
}

void granulator_make_active_track(void *self, int track_num)
{
    granulator *l = (granulator *)self;
    l->cur_sample = track_num;
}

int granulator_get_num_tracks(void *self)
{
    granulator *l = (granulator *)self;
    return l->num_samples;
}

void granulator_refresh_grain_stream(granulator *l)
{
    double beat_time_in_secs = 60.0 / mixr->bpm;
    double seconds_in_cloud_loop = 4 * beat_time_in_secs * l->loop_len;
    l->num_grains_per_looplen = seconds_in_cloud_loop * l->grains_per_sec;
    int grain_duration_samples = l->grain_duration_ms * 44.1;
    int spacing =
        mixr->loop_len_in_samples * l->loop_len / l->num_grains_per_looplen;

    // TODO - don't think grain_stream_len_samples needs to be a member variable
    l->grain_stream_len_samples = mixr->loop_len_in_samples * l->loop_len;
    printf("LOOPLEN:%d NUMGRAINS:%d LENOFGRAINMS:%d, LENOFGRAIN_SAMPLE:%d, "
           "SPACING:%d\n",
           l->grain_stream_len_samples, l->num_grains_per_looplen,
           l->grain_duration_ms, grain_duration_samples, spacing);

    // create all the necessary grains with appropriate starting idx
    int grain_idx = l->granular_file_position;
    if (l->granular_spray > 0)
        grain_idx += rand() % l->granular_spray;
    int attack_time_pct = l->grain_attack_time_pct;
    int release_time_pct = l->grain_release_time_pct;
    for (int i = 0; i < l->num_grains_per_looplen; i++) {
        sound_grain_init(&l->m_grains[i], grain_duration_samples, grain_idx,
                         attack_time_pct, release_time_pct);
    }

    // reset, then populate the grain_stream positions with the
    // assocciated grain number
    for (int i = 0; i < l->grain_stream_len_samples; i++)
        l->grain_stream[i] = -99;
    int current_grain = 0;
    for (int i = 0; i < l->grain_stream_len_samples;) {
        l->grain_stream[i] = current_grain++;
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
    int current_sample_file_size =
        l->samples[l->cur_sample]->resampled_file_size;
    if (pos < current_sample_file_size)
        l->granular_file_position = pos;
    else
        printf("Position must be less than file length:%d\n",
               current_sample_file_size);
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
