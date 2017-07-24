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

    g->grain_file_position = 0;
    g->granular_spray = 441; // 10ms * SR/1000;
    g->grain_duration_ms = 50;
    g->grains_per_sec = 30; // density
    g->grain_attack_time_pct = 2;
    g->grain_release_time_pct = 2;
    g->quasi_grain_fudge = 220; // samples
    g->selection_mode = GRAIN_SELECTION_STATIC;

    g->scan_through_file = false;
    g->scan_speed = 1;

    g->sound_generator.gennext = &granulator_gennext;
    g->sound_generator.status = &granulator_status;
    g->sound_generator.getvol = &granulator_getvol;
    g->sound_generator.setvol = &granulator_setvol;
    g->sound_generator.start = &granulator_start;
    g->sound_generator.stop = &granulator_stop;
    g->sound_generator.get_num_tracks = &granulator_get_num_tracks;
    g->sound_generator.make_active_track = &granulator_make_active_track;
    g->sound_generator.type = GRANULATOR_TYPE;

    granulator_import_file(g, filename);
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

    if (g->scan_through_file) {
        g->grain_file_position += g->scan_speed;
        if (g->grain_file_position >= g->filecontents_len)
            g->grain_file_position =
                g->grain_file_position % g->filecontents_len;
        else if (g->grain_file_position < 0)
            g->grain_file_position =
                g->filecontents_len - g->grain_file_position;
    }

    int cur_loop_position = mixr->cur_sample % mixr->loop_len_in_samples;
    bool new_grain = g->grain_stream[cur_loop_position];
    if (new_grain) {
        g->cur_grain_num = granulator_get_available_grain_num(g);
        if (g->cur_grain_num == -99) {
            printf("PANIC!\n");
            exit(0);
        }

        int duration = g->grain_duration_ms * 44.1;
        int fudge = 0;
        if (g->quasi_grain_fudge != 0)
            fudge = rand() % g->quasi_grain_fudge;
        duration += fudge;

        int grain_idx = g->grain_file_position;
        if (g->selection_mode == GRAIN_SELECTION_RANDOM)
            grain_idx = rand() % (g->filecontents_len - duration);

        if (g->granular_spray > 0)
            grain_idx += rand() % g->granular_spray;

        int attack_time_pct = g->grain_attack_time_pct;
        int release_time_pct = g->grain_release_time_pct;
        sound_grain_init(&g->m_grains[g->cur_grain_num], duration, grain_idx,
                         attack_time_pct, release_time_pct);
        g->num_active_grains++;
        int num_deactivated = granulator_deactivate_other_grains(g);
        g->num_active_grains -= num_deactivated;
    }

    for (int i = 0; i < g->highest_grain_num; i++) {
        int grain_idx = sound_grain_generate_idx(&g->m_grains[i]);
        if (grain_idx != -99) {
            int modified_idx = grain_idx % g->filecontents_len;
            val += g->filecontents[modified_idx] *
                   sound_grain_env(&g->m_grains[i]);
        }
    }

    val = effector(&g->sound_generator, val);
    val = envelopor(&g->sound_generator, val);

    return val * g->vol;
}

void granulator_status(void *self, wchar_t *status_string)
{
    granulator *g = (granulator *)self;
    swprintf(status_string, MAX_PS_STRING_SZ, WCOOL_COLOR_ORANGE
             "[GRANULATOR] vol:%.2lf file:%s len:%d quasi_grain_fudge:%d"
             " grain_duration_ms:%d grains_per_sec:%d grain_spray_ms:%d\n"
             "      grain_file_pos:%d scan:%s scan_speed:%d selection_mode:%d "
             "active_grains:%d",
             g->vol, g->filename, g->filecontents_len, g->quasi_grain_fudge,
             g->grain_duration_ms, g->grains_per_sec, g->granular_spray,
             g->grain_file_position, g->scan_through_file ? "true" : "false",
             g->scan_speed, g->selection_mode, g->num_active_grains);
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

void granulator_import_file(granulator *g, char *filename)
{
    SNDFILE *snd_file;
    SF_INFO sf_info;

    char cwd[1024];
    getcwd(cwd, 1024);
    char full_filename[strlen(cwd) + 7 /* '/wavs/' is 6 and 1 for null */ +
                       strlen(filename)];
    strcpy(full_filename, cwd);
    strcat(full_filename, "/wavs/");
    strcat(full_filename, filename);

    strncpy(g->filename, filename, 512);

    sf_info.format = 0;
    snd_file = sf_open(full_filename, SFM_READ, &sf_info);
    if (!snd_file) {
        printf("Barfed opening %s : %d", full_filename, sf_error(snd_file));
        return;
    }
    g->filecontents_len = sf_info.channels * sf_info.frames;
    printf("Calloc'ing a buffer of %d\n", g->filecontents_len);
    double *filecontents =
        (double *)calloc(g->filecontents_len, sizeof(double));
    if (filecontents == NULL) {
        perror("deid!\n");
        sf_close(snd_file);
        return;
    }
    if (g->filecontents) // already have old contents
        free(g->filecontents);

    g->filecontents = filecontents;
    sf_readf_double(snd_file, g->filecontents, g->filecontents_len);
    sf_close(snd_file);
}

void granulator_refresh_grain_stream(granulator *g)
{
    int looplen_in_seconds = mixr->loop_len_in_samples / (double)SAMPLE_RATE;
    g->num_grains_per_looplen = looplen_in_seconds * g->grains_per_sec;
    int grain_duration_samples =
        g->grain_duration_ms * (double)SAMPLE_RATE / 1000.;
    int spacing = mixr->loop_len_in_samples / g->num_grains_per_looplen;

    memset(g->grain_stream, 0, sizeof(int) * mixr->loop_len_in_samples);
    for (int i = 0; i < mixr->loop_len_in_samples;) {
        g->grain_stream[i] = 1;
        int fudge = rand() % 441;
        i += spacing + fudge;
    }
}

void granulator_set_grain_duration(granulator *g, int dur)
{
    // if (dur < MAX_GRAIN_DURATION) {
    g->grain_duration_ms = dur;
    granulator_refresh_grain_stream(g);
    //} else
    //    printf("Sorry, grain duration must be under %d\n",
    //    MAX_GRAIN_DURATION);
}

void granulator_set_grains_per_sec(granulator *g, int gps)
{
    g->grains_per_sec = gps;
    granulator_refresh_grain_stream(g);
}

void granulator_set_grain_attack_size_pct(granulator *g, int attack_pct)
{
    if (attack_pct < 50)
        g->grain_attack_time_pct = attack_pct;
    granulator_refresh_grain_stream(g);
}

void granulator_set_grain_release_size_pct(granulator *g, int release_pct)
{
    if (release_pct < 50)
        g->grain_release_time_pct = release_pct;
    granulator_refresh_grain_stream(g);
}

void granulator_set_grain_file_position(granulator *g, int pos)
{
    if (pos < 0 || pos > 100) {
        printf("file position should be a percent\n");
        return;
    }
    g->grain_file_position = (double)pos / 100. * g->filecontents_len;
    granulator_refresh_grain_stream(g);
}

void granulator_set_granular_spray(granulator *g, int spray_ms)
{
    int spray_samples = spray_ms * 44.1;
    g->granular_spray = spray_samples;
}

void granulator_set_quasi_grain_fudge(granulator *g, int fudgefactor)
{
    g->quasi_grain_fudge = fudgefactor;
}

void granulator_set_scan_mode(granulator *g, bool b)
{
    if (b != 0 && b != 1) {
        printf("BOOLEY BOOLEY!\n");
        return;
    }
    g->scan_through_file = b;
}

void granulator_set_scan_speed(granulator *g, int speed)
{
    g->scan_speed = speed;
}

void granulator_set_selection_mode(granulator *g, unsigned int mode)
{
    if (mode >= GRAIN_NUM_SELECTION_MODES) {
        printf("Selection must be < %d\n", GRAIN_NUM_SELECTION_MODES);
        return;
    }
    g->selection_mode = mode;
}

int granulator_get_available_grain_num(granulator *g)
{
    int idx = 0;
    while (idx < MAX_CONCURRENT_GRAINS) {
        if (!g->m_grains[idx].active) {
            if (idx > g->highest_grain_num)
                g->highest_grain_num = idx;
            return idx;
        }
        idx++;
    }
    printf("WOW - NO GRAINS TO BE FOUND IN %d attempts\n", idx++);
    return -99;
}

int granulator_deactivate_other_grains(granulator *g)
{
    int num_deactivated = 0;
    for (int i = 0; i < g->highest_grain_num; i++) {
        if (i == g->cur_grain_num)
            continue;
        if (g->m_grains[i].active && !g->m_grains[i].deactivation_pending) {
            g->m_grains[i].deactivation_pending = true;
            num_deactivated++;
        }
    }
    return num_deactivated;
}

//////////////////////////// grain stuff //////////////////////////

void sound_grain_init(sound_grain *g, int dur, int starting_idx, int attack_pct,
                      int release_pct)
{
    g->grain_len_samples = dur;
    g->audiobuffer_start_idx = starting_idx;
    g->audiobuffer_cur_pos = starting_idx;
    g->attack_time_pct = attack_pct;
    g->release_time_pct = release_pct;
    g->active = true;
    g->deactivation_pending = false;
}

int sound_grain_generate_idx(sound_grain *g)
{
    if (!g->active)
        return -99;

    double my_idx = g->audiobuffer_cur_pos;

    g->audiobuffer_cur_pos++;
    if (g->audiobuffer_cur_pos >=
        g->audiobuffer_start_idx + g->grain_len_samples) {
        g->audiobuffer_cur_pos = g->audiobuffer_start_idx;
        if (g->deactivation_pending)
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
