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

static char *s_env_names[] = {"PARABOLIC", "TRAPEZOIDAL", "COSINE"};

granulator *new_granulator(char *filename)
{
    granulator *g = (granulator *)calloc(1, sizeof(granulator));
    g->vol = 0.4;
    g->started = false;
    g->have_active_buffer = false;

    g->audio_buffer_read_idx = 0;
    g->granular_spray_frames = 441; // 10ms * (44100/1000)
    g->grain_duration_ms = 50;
    g->grains_per_sec = 30;
    g->grain_attack_time_pct = 20;
    g->grain_release_time_pct = 20;
    g->quasi_grain_fudge = 220;
    g->selection_mode = GRAIN_SELECTION_STATIC;
    g->envelope_mode = GRANULATOR_ENV_PARABOLIC;
    g->movement_mode = 0; // off or on
    g->reverse_mode = 0;  // off or on
    g->external_source_sg = -1;

    g->loop_mode = false;
    g->loop_len = 1;

    g->grain_pitch = 1;
    g->sequencer_mode = false;

    g->sound_generator.gennext = &granulator_gennext;
    g->sound_generator.status = &granulator_status;
    g->sound_generator.getvol = &granulator_getvol;
    g->sound_generator.setvol = &granulator_setvol;
    g->sound_generator.start = &granulator_start;
    g->sound_generator.stop = &granulator_stop;
    g->sound_generator.get_num_tracks = &granulator_get_num_tracks;
    g->sound_generator.make_active_track = &granulator_make_active_track;
    g->sound_generator.self_destruct = &granulator_del_self;
    g->sound_generator.event_notify = &granulator_event_notify;
    g->sound_generator.type = GRANULATOR_TYPE;

    if (strncmp(filename, "none", 4) != 0)
        granulator_import_file(g, filename);

    seq_init(&g->m_seq);
    granulator_set_sequencer_mode(g, false);

    envelope_generator_init(&g->m_eg1); // start/stop env
    g->m_eg1.m_attack_time_msec = 400;
    g->m_eg1.m_release_time_msec = 750;

    g->graindur_lfo_on = false;
    g->m_lfo1_min = 50;
    g->m_lfo1_max = 80;
    osc_new_settings((oscillator *)&g->m_lfo1);
    lfo_set_soundgenerator_interface(&g->m_lfo1);
    g->m_lfo1.osc.m_osc_fo = 0.01; // default LFO
    g->m_lfo1.osc.m_amplitude = 1.;
    g->lfo1_sync = false;
    lfo_start_oscillator((oscillator *)&g->m_lfo1);

    g->grainps_lfo_on = false;
    g->m_lfo2_min = 50;
    g->m_lfo2_max = 90;
    osc_new_settings((oscillator *)&g->m_lfo2);
    lfo_set_soundgenerator_interface(&g->m_lfo2);
    g->m_lfo2.osc.m_osc_fo = 0.01; // default LFO
    g->m_lfo2.osc.m_amplitude = 1.;
    g->lfo2_sync = false;
    lfo_start_oscillator((oscillator *)&g->m_lfo2);

    g->grainscanfile_lfo_on = false;
    g->m_lfo3_min = 0;
    g->m_lfo3_max = g->audio_buffer_len;
    osc_new_settings((oscillator *)&g->m_lfo3);
    lfo_set_soundgenerator_interface(&g->m_lfo3);
    g->m_lfo3.osc.m_osc_fo = 0.01; // default LFO
    g->m_lfo3.osc.m_amplitude = 1.;
    g->lfo3_sync = false;
    lfo_start_oscillator((oscillator *)&g->m_lfo3);

    g->grainpitch_lfo_on = false;
    g->m_lfo4_min = 0.5;
    g->m_lfo4_max = 1.;
    osc_new_settings((oscillator *)&g->m_lfo4);
    lfo_set_soundgenerator_interface(&g->m_lfo4);
    g->m_lfo4.osc.m_osc_fo = 0.1; // default LFO
    g->m_lfo4.osc.m_amplitude = 1.;
    g->lfo4_sync = false;
    lfo_start_oscillator((oscillator *)&g->m_lfo4);

    granulator_start(g);

    return g;
}

static bool playing = false;
static int playing_midi_tick_count = 0;
void granulator_event_notify(void *self, unsigned int event_type)
{
    granulator *g = (granulator *)self;

    switch (event_type)
    {
    case (TIME_MIDI_TICK):
        if (g->loop_mode)
        {
            int pulses_per_loop = PPBAR * g->loop_len;
            double rel_pos = mixr->timing_info.midi_tick % pulses_per_loop;
            rel_pos = 100. / pulses_per_loop * rel_pos;
            double new_read_idx = g->audio_buffer_len / 100. * rel_pos;
            if (g->num_channels == 2)
                new_read_idx -= ((int)new_read_idx & 1);
            g->audio_buffer_read_idx = new_read_idx;
        }

        if (g->sequencer_mode && g->m_seq.num_patterns > 0)
        {
            int idx = mixr->timing_info.midi_tick % PPBAR;
            if (mixr->timing_info.is_midi_tick &&
                g->m_seq.patterns[g->m_seq.cur_pattern][idx])
            {
                granulator_start(g);
                playing = true;
            }

            seq_tick(&g->m_seq);
        }

        if (playing)
            playing_midi_tick_count++;

        break;
    case (TIME_SIXTEENTH_TICK):
        if (playing && playing_midi_tick_count > PPQN / 2)
        {
            granulator_stop(g);
            playing = false;
            playing_midi_tick_count = 0;
        }
    }
}

void granulator_update_lfos(granulator *g)
{
    osc_update((oscillator *)&g->m_lfo1);
    osc_update((oscillator *)&g->m_lfo2);
    osc_update((oscillator *)&g->m_lfo3);
    osc_update((oscillator *)&g->m_lfo4);

    if (g->graindur_lfo_on)
    {
        double lfo1_out = lfo_do_oscillate((oscillator *)&g->m_lfo1, NULL);
        double scaley_val =
            scaleybum(-1, 1, g->m_lfo1_min, g->m_lfo1_max, lfo1_out);
        g->grain_duration_ms = scaley_val;
    }

    if (g->grainps_lfo_on)
    {
        double lfo2_out = lfo_do_oscillate((oscillator *)&g->m_lfo2, NULL);
        double scaley_val =
            scaleybum(-1, 1, g->m_lfo2_min, g->m_lfo2_max, lfo2_out);
        g->grains_per_sec = scaley_val;
    }

    if (g->grainscanfile_lfo_on)
    {
        double lfo3_out = lfo_do_oscillate((oscillator *)&g->m_lfo3, NULL);
        double scaley_val =
            scaleybum(-1, 1, g->m_lfo3_min, g->m_lfo3_max, lfo3_out);
        double diff;
        if (g->audio_buffer_read_idx > scaley_val)
            diff = g->audio_buffer_read_idx - scaley_val;
        else
            diff = scaley_val - g->audio_buffer_read_idx;

        g->audio_buffer_read_idx += diff;

        if (g->audio_buffer_read_idx >= g->audio_buffer_len)
            g->audio_buffer_read_idx =
                (int)g->audio_buffer_read_idx % g->audio_buffer_len;
        else if (g->audio_buffer_read_idx < 0)
            g->audio_buffer_read_idx =
                g->audio_buffer_len - g->audio_buffer_read_idx;
    }

    if (g->grainpitch_lfo_on)
    {
        double lfo4_out = lfo_do_oscillate((oscillator *)&g->m_lfo4, NULL);
        double scaley_val =
            scaleybum(0, 2, g->m_lfo4_min, g->m_lfo4_max, lfo4_out);
        g->grain_pitch = scaley_val;
    }
}

stereo_val granulator_gennext(void *self)
{
    granulator *g = (granulator *)self;
    stereo_val val = {0., 0.};

    if (!g->sound_generator.active)
        return val;

    if (g->m_eg1.m_state == OFFF)
        g->sound_generator.active = false;

    if (g->external_source_sg != -1)
    {
        g->audio_buffer[g->audio_buffer_write_idx] =
            mixr->soundgen_cur_val[g->external_source_sg].left;
        g->audio_buffer[g->audio_buffer_write_idx + 1] =
            mixr->soundgen_cur_val[g->external_source_sg].right;
        g->audio_buffer_write_idx = g->audio_buffer_write_idx + 2;
        if (g->audio_buffer_write_idx >= g->audio_buffer_len)
            g->audio_buffer_write_idx = 0;
    }

    granulator_update_lfos(g);
    if (g->have_active_buffer) // file buffer or external in
    {
        // STEP 1 - calculate if we should launch a new grain
        int spacing = granulator_calculate_grain_spacing(g);
        if (mixr->timing_info.cur_sample >
            g->last_grain_launched_sample_time + spacing) // new grain time
        {
            g->last_grain_launched_sample_time = mixr->timing_info.cur_sample;
            g->cur_grain_num = granulator_get_available_grain_num(g);

            int duration = g->grain_duration_ms * 44.1;
            if (g->quasi_grain_fudge != 0)
                duration += rand() % g->quasi_grain_fudge;

            int grain_idx = g->audio_buffer_read_idx;
            if (g->selection_mode == GRAIN_SELECTION_RANDOM)
                grain_idx = rand() % (g->audio_buffer_len - duration);

            if (g->granular_spray_frames > 0)
                grain_idx += rand() % g->granular_spray_frames;

            int attack_time_pct = g->grain_attack_time_pct;
            int release_time_pct = g->grain_release_time_pct;

            sound_grain_init(&g->m_grains[g->cur_grain_num], duration,
                             grain_idx, attack_time_pct, release_time_pct,
                             g->reverse_mode, g->grain_pitch, g->num_channels);
            g->num_active_grains = granulator_count_active_grains(g);
        }

        // STEP 2 - gather vals from all active grains
        for (int i = 0; i < g->highest_grain_num; i++)
        {
            stereo_val tmp = sound_grain_generate(
                &g->m_grains[i], g->audio_buffer, g->audio_buffer_len);
            double env = sound_grain_env(&g->m_grains[i], g->envelope_mode);
            val.left += tmp.left * env;
            val.right += tmp.right * env;
        }
    }

    val.left = effector(&g->sound_generator, val.left);
    val.right = effector(&g->sound_generator, val.right);
    val.left = envelopor(&g->sound_generator, val.left);
    val.right = envelopor(&g->sound_generator, val.right);

    eg_update(&g->m_eg1);
    double eg_amp = eg_do_envelope(&g->m_eg1, NULL);

    val.left = val.left * g->vol * eg_amp;
    val.right = val.right * g->vol * eg_amp;

    return val;
}

void granulator_status(void *self, wchar_t *status_string)
{
    granulator *g = (granulator *)self;
    swprintf(status_string, MAX_PS_STRING_SZ,
             L"[GRANULATOR] vol:%.2lf source:%s loop_mode:%s loop_len:%d "
             L"extsource:%d len:%d stereo:%s\n"
             "      audio_buffer_read_idx:%d audio_buffer_write_idx:%d "
             "grain_duration_ms:%d grains_per_sec:%d\n"
             "      quasi_grain_fudge:%d grain_spray_ms:%.2f "
             "active_grains:%d highest_grain_num:%d\n"
             "      selection_mode:%d env_mode:%s movement:%d reverse:%d "
             "sequencer_mode:%s\n"

             "\n      [grain_pitch:%f] grainpitch_lfo_on:%d\n"
             "      lfo4_type:%d lfo4_amp:%f lfo4_rate:%f"
             " lfo4_min:%f lfo4_max:%f \n"

             "\n      [grain_duration_ms:%d] graindur_lfo_on:%d\n"
             "      lfo1_type:%d lfo1_amp:%f lfo1_rate:%f"
             " lfo1_min:%f lfo1_max:%f \n"

             "\n      [grains_per_sec:%d] grainps_lfo_on:%d\n"
             "      lfo2_type:%d lfo2_amp:%f lfo2_rate:%f"
             " lfo2_min:%f lfo2_max:%f \n"

             "\n      grainscan_lfo_on:%d lfo3_type:%d lfo3_amp:%f lfo3_rate:%f"
             " lfo3_min:%f lfo3_max:%f \n"

             "      eg_amp_attack_ms:%.2f eg_amp_release_ms:%.2f eg_state:%d\n",

             g->vol, g->filename, g->loop_mode ? "true" : "false", g->loop_len,
             g->external_source_sg, g->audio_buffer_len,
             g->num_channels == 2 ? "true" : "false",
             (int)g->audio_buffer_read_idx, g->audio_buffer_write_idx,
             g->grain_duration_ms, g->grains_per_sec, g->quasi_grain_fudge,
             g->granular_spray_frames / 44.1, g->num_active_grains,
             g->highest_grain_num, g->selection_mode,
             s_env_names[g->envelope_mode], g->movement_mode, g->reverse_mode,
             g->sequencer_mode ? "true" : "false",

             g->grain_pitch, g->grainpitch_lfo_on, g->m_lfo4.osc.m_waveform,
             g->m_lfo4.osc.m_amplitude, g->m_lfo4.osc.m_osc_fo, g->m_lfo4_min,
             g->m_lfo4_max,

             g->grain_duration_ms, g->graindur_lfo_on, g->m_lfo1.osc.m_waveform,
             g->m_lfo1.osc.m_amplitude, g->m_lfo1.osc.m_osc_fo, g->m_lfo1_min,
             g->m_lfo1_max,

             g->grains_per_sec, g->grainps_lfo_on, g->m_lfo2.osc.m_waveform,
             g->m_lfo2.osc.m_amplitude, g->m_lfo2.osc.m_osc_fo, g->m_lfo2_min,
             g->m_lfo2_max,

             g->grainscanfile_lfo_on, g->m_lfo3.osc.m_waveform,
             g->m_lfo3.osc.m_amplitude, g->m_lfo3.osc.m_osc_fo, g->m_lfo3_min,
             g->m_lfo3_max,

             g->m_eg1.m_attack_time_msec, g->m_eg1.m_release_time_msec,
             g->m_eg1.m_state);

    for (int i = 0; i < g->num_active_grains; i++)
    {
        printf("Grain[%d] active:%s start:%d durInFrames:%d pos:%f incr:%f\n",
               i, g->m_grains[i].active ? "TRUE" : "FALSE",
               g->m_grains[i].audiobuffer_start_idx,
               g->m_grains[i].grain_len_frames,
               g->m_grains[i].audiobuffer_cur_pos, g->m_grains[i].incr);
    }

    wchar_t seq_status_string[MAX_PS_STRING_SZ];
    memset(seq_status_string, 0, MAX_PS_STRING_SZ);
    seq_status(&g->m_seq, seq_status_string);
    wcscat(status_string, seq_status_string);
    wcscat(status_string, WANSI_COLOR_RESET);
}

void granulator_start(void *self)
{
    granulator *g = (granulator *)self;
    eg_start_eg(&g->m_eg1);
    g->sound_generator.active = true;
}

void granulator_stop(void *self)
{
    granulator *g = (granulator *)self;
    eg_release(&g->m_eg1);
}

double granulator_getvol(void *self)
{
    granulator *g = (granulator *)self;
    return g->vol;
}

void granulator_setvol(void *self, double v)
{
    granulator *g = (granulator *)self;
    if (v < 0.0 || v > 1.0)
    {
        return;
    }
    g->vol = v;
}

void granulator_del_self(void *self)
{
    // TODO delete file
    granulator *g = (granulator *)self;
    free(g);
}

void granulator_make_active_track(void *self, int track_num)
{
    // NOOP
    (void)self;
    (void)track_num;
}

int granulator_get_num_tracks(void *self)
{
    (void)self;
    return 1;
}

//////////////////////////// grain stuff //////////////////////////
// granulator functions contuine below

void sound_grain_init(sound_grain *g, int dur, int starting_idx, int attack_pct,
                      int release_pct, bool reverse_mode, double pitch,
                      int num_channels)
{
    g->audiobuffer_start_idx = starting_idx;
    g->grain_len_frames = dur;
    g->attack_time_pct = attack_pct;
    g->release_time_pct = release_pct;
    g->audiobuffer_num_channels = num_channels;

    g->reverse_mode = reverse_mode;
    if (reverse_mode)
    {
        g->audiobuffer_cur_pos = starting_idx + (dur * num_channels) - 1;
        g->incr = -1.0 * pitch;
    }
    else
    {
        g->audiobuffer_cur_pos = starting_idx;
        g->incr = pitch;
    }

    g->attack_time_samples = dur / 100. * attack_pct;
    g->release_time_samples = dur / 100. * release_pct;

    g->amp = 0;
    double rdur = 1.0 / dur;
    double rdur2 = rdur * rdur;
    g->slope = 4.0 * 1.0 * (rdur - rdur2);
    g->curve = -8.0 * 1.0 * rdur2;

    g->active = true;
}

static inline void sound_grain_check_idx(int *index, int buffer_len)
{
    while (*index < 0.0)
        *index += buffer_len;
    while (*index >= buffer_len)
        *index -= buffer_len;
}

stereo_val sound_grain_generate(sound_grain *g, double *audio_buffer,
                                int audio_buffer_len)
{
    stereo_val out = {0., 0.};
    if (!g->active)
        return out;

    int num_channels = g->audiobuffer_num_channels;

    int read_idx = (int)g->audiobuffer_cur_pos;

    sound_grain_check_idx(&read_idx, audio_buffer_len);
    out.left = audio_buffer[read_idx];
    if (num_channels > 1)
    {
        int read_idx_right = read_idx + 1;
        sound_grain_check_idx(&read_idx_right, audio_buffer_len);
        out.right = audio_buffer[read_idx_right];
    }
    else
        out.right = out.left;

    g->audiobuffer_cur_pos += (g->incr * num_channels);

    int end_buffer =
        g->audiobuffer_start_idx + (g->grain_len_frames * num_channels);

    if ((g->reverse_mode &&
         g->audiobuffer_cur_pos < g->audiobuffer_start_idx) ||
        g->audiobuffer_cur_pos >= end_buffer)
    {
        g->active = false;
    }

    return out;
}

double sound_grain_env(sound_grain *g, unsigned int envelope_mode)
{
    double amp = 0;
    int idx = g->audiobuffer_cur_pos;
    double percent_pos = 0;

    switch (envelope_mode)
    {
    case (GRANULATOR_ENV_PARABOLIC):
        g->amp = g->amp + g->slope;
        g->slope = g->slope + g->curve;
        amp = g->amp;
        break;
    case (GRANULATOR_ENV_TRAPEZOIDAL):
        amp = 1;
        percent_pos =
            100. / g->grain_len_frames * (idx - g->audiobuffer_start_idx);
        if (percent_pos < g->attack_time_pct)
            amp *= percent_pos / g->attack_time_pct;
        else if (percent_pos > (100 - g->release_time_pct))
            amp *= (100 - percent_pos) / g->release_time_pct;
        break;
    }

    return amp;
}

//////////////////////////// end of grain stuff //////////////////////////

void granulator_import_file(granulator *g, char *filename)
{
    strncpy(g->filename, filename, 512);
    audio_buffer_details deetz =
        import_file_contents(&g->audio_buffer, filename);
    g->audio_buffer_len = deetz.buffer_length;
    g->num_channels = deetz.num_channels;
    g->external_source_sg = -1;
    g->have_active_buffer = true;
}

void granulator_set_external_source(granulator *g, int sound_gen_num)
{
    if (mixer_is_valid_soundgen_num(mixr, sound_gen_num))
    {
        g->external_source_sg = sound_gen_num;
        int looplen = mixr->timing_info.loop_len_in_frames * 2; // stereo
        double *buffer = calloc(looplen, sizeof(double));
        if (buffer)
        {
            if (g->audio_buffer)
                free(g->audio_buffer);
            g->audio_buffer = buffer;
            g->audio_buffer_len = looplen;
        }
    }
    g->have_active_buffer = true;
}

int granulator_calculate_grain_spacing(granulator *g)
{
    int looplen_in_seconds =
        mixr->timing_info.loop_len_in_frames / (double)SAMPLE_RATE;
    g->num_grains_per_looplen = looplen_in_seconds * g->grains_per_sec;
    if (g->num_grains_per_looplen == 0)
    {
        g->num_grains_per_looplen = 2; // whoops! dn't wanna div by 0 below
    }
    int spacing =
        mixr->timing_info.loop_len_in_frames / g->num_grains_per_looplen;
    return spacing;
}

void granulator_set_grain_duration(granulator *g, int dur)
{
    // if (dur < MAX_GRAIN_DURATION) {
    g->grain_duration_ms = dur;
    //} else
    //    printf("Sorry, grain duration must be under %d\n",
    //    MAX_GRAIN_DURATION);
}

void granulator_set_grains_per_sec(granulator *g, int gps)
{
    g->grains_per_sec = gps;
}

void granulator_set_grain_attack_size_pct(granulator *g, int attack_pct)
{
    if (attack_pct < 50)
        g->grain_attack_time_pct = attack_pct;
}

void granulator_set_grain_release_size_pct(granulator *g, int release_pct)
{
    if (release_pct < 50)
        g->grain_release_time_pct = release_pct;
}

void granulator_set_audio_buffer_read_idx(granulator *g, int pos)
{
    if (pos < 0 || pos >= g->audio_buffer_len)
    {
        return;
    }
    g->audio_buffer_read_idx = pos;
}

void granulator_set_granular_spray(granulator *g, int spray_ms)
{
    int spray_frames = spray_ms * 44.1;
    g->granular_spray_frames = spray_frames;
}

void granulator_set_quasi_grain_fudge(granulator *g, int fudgefactor)
{
    g->quasi_grain_fudge = fudgefactor;
}

void granulator_set_grain_pitch(granulator *g, double pitch)
{
    g->grain_pitch = pitch;
}

void granulator_set_selection_mode(granulator *g, unsigned int mode)
{
    if (mode >= GRAIN_NUM_SELECTION_MODES)
    {
        printf("Selection must be < %d\n", GRAIN_NUM_SELECTION_MODES);
        return;
    }
    g->selection_mode = mode;
}

void granulator_set_envelope_mode(granulator *g, unsigned int mode)
{
    if (mode >= GRANULATOR_ENV_NUM)
    {
        printf("Selection must be < %d\n", GRANULATOR_ENV_NUM);
        return;
    }
    g->envelope_mode = mode;
}

void granulator_set_reverse_mode(granulator *g, bool b) { g->reverse_mode = b; }
void granulator_set_movement_mode(granulator *g, bool b)
{
    g->movement_mode = b;
}
void granulator_set_loop_mode(granulator *g, bool b)
{
    g->loop_mode = b;
    g->quasi_grain_fudge = 0;
    g->selection_mode = GRAIN_SELECTION_STATIC;
    g->granular_spray_frames = 0;
    g->grain_duration_ms = 100;
}

void granulator_set_loop_len(granulator *g, int bars)
{
    if (bars != 0)
        g->loop_len = bars;
}

void granulator_set_sequencer_mode(granulator *g, bool b)
{
    g->sequencer_mode = b;
    if (b)
        granulator_stop(g); // stop so that env fades
    else
        granulator_start(g);
}

int granulator_get_available_grain_num(granulator *g)
{
    int idx = 0;
    while (idx < MAX_CONCURRENT_GRAINS)
    {
        if (!g->m_grains[idx].active)
        {
            if (idx > g->highest_grain_num)
                g->highest_grain_num = idx;
            return idx;
        }
        idx++;
    }
    printf("WOW - NO GRAINS TO BE FOUND IN %d attempts\n", idx++);
    return 0;
}

int granulator_count_active_grains(granulator *g)
{
    int active = 0;
    for (int i = 0; i < g->highest_grain_num; i++)
        if (g->m_grains[i].active)
            active++;

    return active;
}

void granulator_set_lfo_amp(granulator *g, int lfonum, double amp)
{
    if (amp >= 0.0 && amp <= 1.0)
    {
        switch (lfonum)
        {
        case (1):
            g->m_lfo1.osc.m_amplitude = amp;
            break;
        case (2):
            g->m_lfo2.osc.m_amplitude = amp;
            break;
        case (3):
            g->m_lfo3.osc.m_amplitude = amp;
            break;
        case (4):
            g->m_lfo4.osc.m_amplitude = amp;
            break;
        }
    }
    else
        printf("Amp should be between 0 and 1\n");
}

void granulator_set_lfo_voice(granulator *g, int lfonum, unsigned int voice)
{
    if (voice < MAX_LFO_OSC)
    {
        switch (lfonum)
        {
        case (1):
            g->m_lfo1.osc.m_waveform = voice;
            break;
        case (2):
            g->m_lfo2.osc.m_waveform = voice;
            break;
        case (3):
            g->m_lfo3.osc.m_waveform = voice;
            break;
        case (4):
            g->m_lfo4.osc.m_waveform = voice;
            break;
        }
    }
    else
        printf("Voice ENUM should be < %d\n", MAX_LFO_OSC);
}

void granulator_set_lfo_rate(granulator *g, int lfonum, double rate)
{
    if (rate >= 0 && rate <= MAX_LFO_RATE)
    {
        switch (lfonum)
        {
        case (1):
            g->m_lfo1.osc.m_osc_fo = rate;
            g->lfo1_sync = false;
            break;
        case (2):
            g->m_lfo2.osc.m_osc_fo = rate;
            g->lfo2_sync = false;
            break;
        case (3):
            g->m_lfo3.osc.m_osc_fo = rate;
            g->lfo3_sync = false;
            break;
        case (4):
            g->m_lfo4.osc.m_osc_fo = rate;
            g->lfo4_sync = false;
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
    switch (lfonum)
    {
    case (1):
        g->m_lfo1_min = minval;
        break;
    case (2):
        g->m_lfo2_min = minval;
        break;
    case (3):
        g->m_lfo3_min = minval;
        break;
    case (4):
        g->m_lfo4_min = minval;
        break;
    }
}

void granulator_set_lfo_max(granulator *g, int lfonum, double maxval)
{
    switch (lfonum)
    {
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
    case (4):
        if (maxval > g->m_lfo4_min)
            g->m_lfo4_max = maxval;
        break;
    }
}

void granulator_set_lfo_sync(granulator *g, int lfonum, int numloops)
{

    int looplen_in_samples = 60 / mixr->bpm * SAMPLE_RATE;
    double osc_fo = (double)SAMPLE_RATE / (looplen_in_samples * numloops);
    printf("Setting LFO %d sync to %f\n", lfonum, osc_fo);
    switch (lfonum)
    {
    case (1):
        g->m_lfo1.osc.m_osc_fo = osc_fo;
        g->lfo1_sync = true;
        break;
    case (2):
        g->m_lfo2.osc.m_osc_fo = osc_fo;
        g->lfo2_sync = true;
        break;
    case (3):
        g->m_lfo3.osc.m_osc_fo = osc_fo;
        g->lfo3_sync = true;
        break;
    case (4):
        g->m_lfo4.osc.m_osc_fo = osc_fo;
        g->lfo4_sync = true;
        break;
    }
}
