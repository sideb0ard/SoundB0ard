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
extern char *s_lfo_mode_names;

granulator *new_granulator(char *filename)
{
    granulator *g = (granulator *)calloc(1, sizeof(granulator));
    g->vol = 0.7;
    g->active = true;
    g->started = false;

    g->grain_file_position = 0;
    g->granular_spray = 0; // 10ms * SR/1000;
    g->grain_duration_ms = 50;
    g->grains_per_sec = 30; // density
    g->grain_attack_time_pct = 2;
    g->grain_release_time_pct = 2;
    g->quasi_grain_fudge = 0; // samples
    g->selection_mode = GRAIN_SELECTION_STATIC;

    g->scan_through_file = false;
    g->scan_speed = 1;
    g->sequencer_mode = false;

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
    // granulator_refresh_grain_stream(g);

    seq_init(&g->m_seq);
    granulator_set_sequencer_mode(g, false);

    envelope_generator_init(&g->m_eg1);

    g->graindur_lfo_on = false;
    g->m_lfo1_min = 20;
    g->m_lfo1_max = 100;
    osc_new_settings((oscillator *)&g->m_lfo1);
    lfo_set_soundgenerator_interface(&g->m_lfo1);
    g->m_lfo1.osc.m_osc_fo = 0.01; // default LFO
    g->m_lfo1.osc.m_amplitude = 1.;
    lfo_start_oscillator((oscillator *)&g->m_lfo1);

    g->grainps_lfo_on = false;
    g->m_lfo2_min = 1;
    g->m_lfo2_max = 100;
    osc_new_settings((oscillator *)&g->m_lfo2);
    lfo_set_soundgenerator_interface(&g->m_lfo2);
    g->m_lfo2.osc.m_osc_fo = 0.01; // default LFO
    g->m_lfo2.osc.m_amplitude = 1.;
    lfo_start_oscillator((oscillator *)&g->m_lfo2);

    g->grainscanfile_lfo_on = false;
    g->m_lfo3_min = 0;
    g->m_lfo3_max = g->filecontents_len;
    osc_new_settings((oscillator *)&g->m_lfo3);
    lfo_set_soundgenerator_interface(&g->m_lfo3);
    g->m_lfo3.osc.m_osc_fo = 0.01; // default LFO
    g->m_lfo3.osc.m_amplitude = 1.;
    lfo_start_oscillator((oscillator *)&g->m_lfo3);

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
        g->grain_file_position++;
        if (g->grain_file_position >= g->filecontents_len)
            g->grain_file_position =
                g->grain_file_position % g->filecontents_len;
        else if (g->grain_file_position < 0)
            g->grain_file_position =
                g->filecontents_len - g->grain_file_position;
    }

    eg_update(&g->m_eg1);
    osc_update((oscillator *)&g->m_lfo1);
    osc_update((oscillator *)&g->m_lfo2);
    osc_update((oscillator *)&g->m_lfo3);

    double eg = eg_do_envelope(&g->m_eg1, NULL);

    if (g->graindur_lfo_on) {
        double lfo1_out = lfo_do_oscillate((oscillator *)&g->m_lfo1, NULL);
        double scaley_val =
            scaleybum(-1, 1, g->m_lfo1_min, g->m_lfo1_max, lfo1_out);
        g->grain_duration_ms = scaley_val;
    }

    if (g->grainps_lfo_on) {
        double lfo2_out = lfo_do_oscillate((oscillator *)&g->m_lfo2, NULL);
        double scaley_val =
            scaleybum(-1, 1, g->m_lfo2_min, g->m_lfo2_max, lfo2_out);
        g->grains_per_sec = scaley_val;
    }

    if (g->grainscanfile_lfo_on) {
        double lfo3_out = lfo_do_oscillate((oscillator *)&g->m_lfo3, NULL);
        double scaley_val =
            scaleybum(-1, 1, g->m_lfo3_min, g->m_lfo3_max, lfo3_out);
        g->grain_file_position = scaley_val;
    }

    int spacing = granulator_calculate_grain_spacing(g);
    if (mixr->cur_sample >
        g->last_grain_launched_sample_time + spacing) // new grain time
    {
        g->last_grain_launched_sample_time = mixr->cur_sample;
        g->cur_grain_num = granulator_get_available_grain_num(g);

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
                         attack_time_pct, release_time_pct, g->scan_speed);
        g->num_active_grains++;
        int num_deactivated = granulator_deactivate_other_grains(g);
        g->num_active_grains -= num_deactivated;
    }

    for (int i = 0; i < g->highest_grain_num; i++) {
        int grain_idx = sound_grain_generate_idx(&g->m_grains[i]);
        if (grain_idx != -99) {
            int modified_idx = grain_idx % g->filecontents_len;
            val += g->filecontents[modified_idx] *
                   sound_grain_env(&g->m_grains[i],
                                   0 /* zero means first grain idx*/);
        }
        // grain_idx = sound_grain_gen_doppelganger_idx(&g->m_grains[i]);
        // if (grain_idx != -99) {
        //    int modified_idx = grain_idx % g->filecontents_len;
        //    val += g->filecontents[modified_idx] *
        //           sound_grain_env(&g->m_grains[i], 1 /* zero means
        //           doppelganger grain idx*/);
        //}
    }

    if (g->sequencer_mode && g->m_seq.num_patterns > 0) {
        int idx = mixr->midi_tick % PPBAR;
        if (mixr->is_midi_tick &&
            g->m_seq.patterns[g->m_seq.cur_pattern][idx]) {
            g->sequencer_gate = 1;
        }
        else if (mixr->is_sixteenth)
            g->sequencer_gate = 0;

        seq_tick(&g->m_seq);
    }

    val = effector(&g->sound_generator, val);
    val = envelopor(&g->sound_generator, val);

    return val * g->vol * g->sequencer_gate;
}

void granulator_status(void *self, wchar_t *status_string)
{
    granulator *g = (granulator *)self;
    swprintf(status_string, MAX_PS_STRING_SZ, WCOOL_COLOR_ORANGE
             "[GRANULATOR] vol:%.2lf file:%s len:%d quasi_grain_fudge:%d"
             " grain_duration_ms:%d grains_per_sec:%d grain_spray_ms:%d\n"
             "      grain_file_pos:%d scan:%s scan_speed:%d selection_mode:%d "
             "active_grains:%d highest_grain_num:%d sequencer_mode:%s\n"
             "      graindur_lfo_on :%s lfo1_type:%d lfo1_amp:%f lfo1_rate:%f"
             " lfo1_min:%f lfo1_max:%f \n"
             "      grainps_lfo_on  :%s lfo2_type:%d lfo2_amp:%f lfo2_rate:%f"
             " lfo2_min:%f lfo2_max:%f \n"
             "      grainscan_lfo_on:%s lfo3_type:%d lfo3_amp:%f lfo3_rate:%f"
             " lfo3_min:%f lfo3_max:%f ",
             g->vol, g->filename, g->filecontents_len, g->quasi_grain_fudge,
             g->grain_duration_ms, g->grains_per_sec, g->granular_spray,
             g->grain_file_position, g->scan_through_file ? "true" : "false",
             g->scan_speed, g->selection_mode, g->num_active_grains,
             g->highest_grain_num, g->sequencer_mode ? "true" : "false",
             // s_lfo_mode_names[g->m_lfo1.osc.m_waveform],
             g->graindur_lfo_on ? "true" : "false", g->m_lfo1.osc.m_waveform,
             g->m_lfo1.osc.m_amplitude, g->m_lfo1.osc.m_osc_fo, g->m_lfo1_min,
             g->m_lfo1_max, g->grainps_lfo_on ? "true" : "false",
             g->m_lfo2.osc.m_waveform, g->m_lfo2.osc.m_amplitude,
             g->m_lfo2.osc.m_osc_fo, g->m_lfo2_min, g->m_lfo2_max,
             g->grainscanfile_lfo_on ? "true" : "false",
             g->m_lfo3.osc.m_waveform, g->m_lfo3.osc.m_amplitude,
             g->m_lfo3.osc.m_osc_fo, g->m_lfo3_min, g->m_lfo3_max);

    wchar_t seq_status_string[MAX_PS_STRING_SZ];
    memset(seq_status_string, 0, MAX_PS_STRING_SZ);
    seq_status(&g->m_seq, seq_status_string);
    wcscat(status_string, seq_status_string);
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

//////////////////////////// grain stuff //////////////////////////
// granulator functions contuine below

void sound_grain_init(sound_grain *g, int dur, int starting_idx, int attack_pct,
                      int release_pct, int pitch)
{
    g->grain_len_samples = dur;
    g->audiobuffer_start_idx = starting_idx;
    g->audiobuffer_cur_pos = starting_idx;
    g->audiobuffer_pitch = pitch;
    g->attack_time_pct = attack_pct;
    g->release_time_pct = release_pct;
    g->active = true;
    g->doppelganger_started =
        false; // start a second grain env, half way through
    g->doppelganger_idx = starting_idx + (dur / 2);
    g->deactivation_pending = false;
}

int sound_grain_generate_idx(sound_grain *g)
{
    if (!g->active)
        return -99;

    double my_idx = g->audiobuffer_cur_pos;

    g->audiobuffer_cur_pos += g->audiobuffer_pitch;
    int end_buffer = g->audiobuffer_start_idx + g->grain_len_samples;
    if (g->audiobuffer_cur_pos >= end_buffer) {
        g->audiobuffer_cur_pos -= g->grain_len_samples;
        if (g->deactivation_pending)
            g->active = false;
    }
    else if (g->audiobuffer_cur_pos == g->grain_len_samples / 2) {
        g->doppelganger_started = true;
    }
    else if (g->audiobuffer_cur_pos < 0) {
        g->audiobuffer_cur_pos = end_buffer - g->audiobuffer_cur_pos;
        if (g->deactivation_pending)
            g->active = false;
    }

    if (g->doppelganger_started) {
        g->doppelganger_idx += g->audiobuffer_pitch;
        if (g->doppelganger_idx >= end_buffer)
            g->doppelganger_idx -= g->grain_len_samples;
        else if (g->doppelganger_idx < 0)
            g->doppelganger_idx += g->grain_len_samples;
    }
    return my_idx;
}

int sound_grain_gen_doppelganger_idx(sound_grain *g)
{
    if (!g->doppelganger_started)
        return -99;
    else
        return g->doppelganger_idx;
}

void sound_grain_reset(sound_grain *g)
{
    g->audiobuffer_cur_pos = g->audiobuffer_start_idx;
}

double sound_grain_env(sound_grain *g, int idx_num)
{
    int idx = 0;
    if (idx_num == 0)
        idx = g->audiobuffer_cur_pos;
    else if (idx_num == 1)
        idx = g->doppelganger_idx;

    double env_amp = 1.;
    double percent_pos =
        100. / g->grain_len_samples * (idx - g->audiobuffer_start_idx);
    if (percent_pos < g->attack_time_pct)
        env_amp *= percent_pos / g->attack_time_pct;
    else if (percent_pos > (100 - g->release_time_pct))
        env_amp *= (100 - percent_pos) / g->release_time_pct;
    return env_amp;
}

//////////////////////////// end of grain stuff //////////////////////////

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

int granulator_calculate_grain_spacing(granulator *g)
{
    int looplen_in_seconds = mixr->loop_len_in_samples / (double)SAMPLE_RATE;
    g->num_grains_per_looplen = looplen_in_seconds * g->grains_per_sec;
    int grain_duration_samples =
        g->grain_duration_ms * (double)SAMPLE_RATE / 1000.;
    int spacing = mixr->loop_len_in_samples / g->num_grains_per_looplen;
    return spacing;
}

void granulator_set_grain_duration(granulator *g, int dur)
{
    // if (dur < MAX_GRAIN_DURATION) {
    g->grain_duration_ms = dur;
    // granulator_refresh_grain_stream(g);
    //} else
    //    printf("Sorry, grain duration must be under %d\n",
    //    MAX_GRAIN_DURATION);
}

void granulator_set_grains_per_sec(granulator *g, int gps)
{
    g->grains_per_sec = gps;
    // granulator_refresh_grain_stream(g);
}

void granulator_set_grain_attack_size_pct(granulator *g, int attack_pct)
{
    if (attack_pct < 50)
        g->grain_attack_time_pct = attack_pct;
    // granulator_refresh_grain_stream(g);
}

void granulator_set_grain_release_size_pct(granulator *g, int release_pct)
{
    if (release_pct < 50)
        g->grain_release_time_pct = release_pct;
    // granulator_refresh_grain_stream(g);
}

void granulator_set_grain_file_position(granulator *g, int pos)
{
    if (pos < 0 || pos > 100) {
        printf("file position should be a percent\n");
        return;
    }
    g->grain_file_position = (double)pos / 100. * g->filecontents_len;
    // granulator_refresh_grain_stream(g);
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

void granulator_set_sequencer_mode(granulator *g, bool b)
{
    if (b != 0 && b != 1) {
        printf("not BOOLEY BOOLEY!\n");
        return;
    }
    if (b)
        g->sequencer_gate = 0;
    else
        g->sequencer_gate = 1;
    g->sequencer_mode = b;
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
    return 0;
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

void granulator_set_lfo_amp(granulator *g, int lfonum, double amp)
{
    if (amp >= 0.0 && amp <= 1.0) {
        switch (lfonum) {
        case (1):
            g->m_lfo1.osc.m_amplitude = amp;
            break;
        case (2):
            g->m_lfo2.osc.m_amplitude = amp;
            break;
        case (3):
            g->m_lfo3.osc.m_amplitude = amp;
            break;
        }
    }
    else
        printf("Amp should be between 0 and 1\n");
}

void granulator_set_lfo_voice(granulator *g, int lfonum, unsigned int voice)
{
    if (voice < MAX_LFO_OSC) {
        switch (lfonum) {
        case (1):
            g->m_lfo1.osc.m_waveform = voice;
            break;
        case (2):
            g->m_lfo2.osc.m_waveform = voice;
            break;
        case (3):
            g->m_lfo3.osc.m_waveform = voice;
            break;
        }
    }
    else
        printf("Voice ENUM should be < %d\n", MAX_LFO_OSC);
}

void granulator_set_lfo_rate(granulator *g, int lfonum, double rate)
{
    // if (rate >= MIN_LFO_RATE && rate <= MAX_LFO_RATE)
    if (rate >= 0 && rate <= MAX_LFO_RATE) {
        switch (lfonum) {
        case (1):
            g->m_lfo1.osc.m_osc_fo = rate;
            break;
        case (2):
            g->m_lfo2.osc.m_osc_fo = rate;
            break;
        case (3):
            g->m_lfo3.osc.m_osc_fo = rate;
            break;
        }
    }
    else
        printf("LFO rate should be between %f and %f\n", 0., MAX_LFO_RATE);
}

void granulator_set_lfo_min(granulator *g, int lfonum, double minval)
{
    if (minval < 0)
        return;
    switch (lfonum) {
    case (1):
        g->m_lfo1_min = minval;
        break;
    case (2):
        g->m_lfo2_min = minval;
        break;
    case (3):
        g->m_lfo3_min = minval;
        break;
    }
}

void granulator_set_lfo_max(granulator *g, int lfonum, double maxval)
{
    switch (lfonum) {
    case (1):
        if (maxval > g->m_lfo1_min)
            g->m_lfo1_max = maxval;
        break;
    case (2):
        if (maxval > g->m_lfo2_min)
            g->m_lfo2_max = maxval;
        break;
    case (3):
        if (maxval > g->m_lfo3_min)
            g->m_lfo3_max = maxval;
        break;
    }
}
