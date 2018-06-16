#include <libgen.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "defjams.h"
#include "looper.h"
#include "mixer.h"
#include "utils.h"
#include <pattern_parser.h>

extern mixer *mixr;
extern char *s_lfo_mode_names;

static char *s_env_names[] = {"PARABOLIC", "TRAPEZOIDAL", "TUKEY"};
static char *s_loop_mode_names[] = {"LOOP", "STATIC"};

looper *new_looper(char *filename)
{
    looper *g = (looper *)calloc(1, sizeof(looper));
    g->vol = 0.4;
    g->have_active_buffer = false;

    g->audio_buffer_read_idx = 0;
    g->granular_spray_frames = 441; // 10ms * (44100/1000)
    g->grain_attack_time_pct = 2;
    g->grain_release_time_pct = 5;
    g->quasi_grain_fudge = 220;
    g->selection_mode = GRAIN_SELECTION_STATIC;
    g->envelope_mode = LOOPER_ENV_PARABOLIC;
    g->envelope_taper_ratio = 0.5;
    g->reverse_mode = 0; // bool
    g->external_source_sg = -1;
    g->buffer_is_full = false;
    g->should_start_recording = false;

    g->loop_mode = LOOPER_LOOP_MODE;
    g->loop_len = 1;
    g->scramble_mode = false;
    g->stutter_mode = false;
    g->stutter_idx = 0;

    g->grain_pitch = 1;

    g->density_duration_sync = true;
    g->fill_factor = 2.;
    looper_set_grain_density(g, 30);

    g->sound_generator.gennext = &looper_gennext;
    g->sound_generator.status = &looper_status;
    g->sound_generator.getvol = &looper_getvol;
    g->sound_generator.setvol = &looper_setvol;
    g->sound_generator.start = &looper_start;
    g->sound_generator.stop = &looper_stop;
    g->sound_generator.get_num_patterns = &looper_get_num_patterns;
    g->sound_generator.set_num_patterns = &looper_set_num_patterns;
    g->sound_generator.make_active_track = &looper_make_active_track;
    g->sound_generator.self_destruct = &looper_del_self;
    g->sound_generator.event_notify = &looper_event_notify;
    g->sound_generator.get_pattern = &looper_get_pattern;
    g->sound_generator.set_pattern = &looper_set_pattern;
    g->sound_generator.is_valid_pattern = &looper_is_valid_pattern;
    g->sound_generator.type = LOOPER_TYPE;

    if (strncmp(filename, "none", 4) != 0)
        looper_import_file(g, filename);

    step_init(&g->m_seq);

    envelope_generator_init(&g->m_eg1); // start/stop env
    g->m_eg1.m_attack_time_msec = 10;
    g->m_eg1.m_release_time_msec = 50;

    // Grain Duration
    g->graindur_lfo_on = false;
    g->m_lfo1_min = 10;
    g->m_lfo1_max = 80;
    osc_new_settings((oscillator *)&g->m_lfo1);
    lfo_set_soundgenerator_interface(&g->m_lfo1);
    g->m_lfo1.osc.m_osc_fo = 0.01; // default LFO
    g->m_lfo1.osc.m_amplitude = 1.;
    g->lfo1_sync = false;
    lfo_start_oscillator((oscillator *)&g->m_lfo1);

    // Grain Density / Grains Per Second
    g->grainps_lfo_on = false;
    g->m_lfo2_min = 20;
    g->m_lfo2_max = 50;
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
    g->m_lfo4_min = 0.9;
    g->m_lfo4_max = 1.1;
    osc_new_settings((oscillator *)&g->m_lfo4);
    lfo_set_soundgenerator_interface(&g->m_lfo4);
    g->m_lfo4.osc.m_osc_fo = 0.1; // default LFO
    g->m_lfo4.osc.m_amplitude = 1.;
    g->lfo4_sync = false;
    lfo_start_oscillator((oscillator *)&g->m_lfo4);

    g->gate_mode = false;
    g->sustain_ms = 50;

    looper_start(g);

    return g;
}

bool looper_is_valid_pattern(void *self, int pattern_num)
{
    looper *l = (looper *)self;
    return step_is_valid_pattern_num(&l->m_seq, pattern_num);
}

midi_event *looper_get_pattern(void *self, int pattern_num)
{
    looper *l = (looper *)self;
    return step_get_pattern(&l->m_seq, pattern_num);
}

void looper_set_pattern(void *self, int pattern_num, midi_event *pattern)
{
    looper *l = (looper *)self;
    return step_set_pattern(&l->m_seq, pattern_num, pattern);
}

static bool playing = false;
static int playing_midi_tick_count = 0;
void looper_event_notify(void *self, unsigned int event_type)
{
    looper *g = (looper *)self;

    switch (event_type)
    {
    case (TIME_START_OF_LOOP_TICK):

        g->started = true;
        g->step_diff = 0;

        if (g->scramble_pending)
        {
            g->scramble_mode = true;
            g->scramble_pending = false;
        }
        else
            g->scramble_mode = false;

        if (g->stutter_pending)
        {
            g->stutter_mode = true;
            g->stutter_pending = false;
        }
        else
            g->stutter_mode = false;

        if (!g->should_start_recording)
            g->should_start_recording = true;

        break;

    case (TIME_MIDI_TICK):
        if (g->loop_mode != LOOPER_STATIC_MODE)
        {
            int pulses_per_loop = PPBAR * g->loop_len;

            double rel_pos = mixr->timing_info.midi_tick % pulses_per_loop;
            rel_pos = 100. / pulses_per_loop * rel_pos;
            double new_read_idx = g->audio_buffer_len / 100. * rel_pos;

            if (g->reverse_mode)
                new_read_idx = (g->audio_buffer_len - 1) - new_read_idx;
            if (g->num_channels == 2)
                new_read_idx -= ((int)new_read_idx & 1);

            if (g->scramble_mode)
            {
                g->audio_buffer_read_idx =
                    new_read_idx + (g->scramble_diff * g->size_of_sixteenth);
            }
            else if (g->stutter_mode)
            {
                int cur_sixteenth = mixr->timing_info.sixteenth_note_tick % 16;
                int rel_pos_within_a_sixteenth =
                    new_read_idx - (cur_sixteenth * g->size_of_sixteenth);
                g->audio_buffer_read_idx =
                    (g->stutter_idx * g->size_of_sixteenth) +
                    rel_pos_within_a_sixteenth;
            }
            else
            {
                int idx = mixr->timing_info.midi_tick % PPBAR;
                if (g->m_seq.patterns[g->m_seq.cur_pattern][idx].event_type ==
                    MIDI_ON)
                {
                    int cur_sixteenth =
                        mixr->timing_info.sixteenth_note_tick % 16;
                    g->step_diff = 0 - cur_sixteenth;
                    looper_set_reverse_mode(g, false);

                    int randy = rand() % 100;
                    if (randy < 20)
                        g->step_diff = 4 - cur_sixteenth;
                    else if (randy < 50)
                        g->step_diff = 8 - cur_sixteenth;
                    else if (randy < 55)
                        g->step_diff = 12 - cur_sixteenth;
                    else if (randy < 60)
                        looper_set_reverse_mode(g, true);

                    if (g->gate_mode)
                    {
                        midi_event *pattern =
                            g->m_seq.patterns[g->m_seq.cur_pattern];
                        int off_tick =
                            (int)(idx + (g->sustain_ms *
                                         mixr->timing_info.midi_ticks_per_ms)) %
                            PPBAR;
                        midi_event off_event = new_midi_event(MIDI_OFF, 0, 128);
                        pattern_add_event(pattern, off_tick, off_event);
                        looper_start(g);
                        // printf("[%d] ON idx is %d -- setting off to "
                        //       "off_tick:%d\n",
                        //       idx, (int)new_read_idx, off_tick);
                    }
                }
                else if (g->m_seq.patterns[g->m_seq.cur_pattern][idx]
                                 .event_type == MIDI_OFF &&
                         g->gate_mode)
                {
                    // printf("[%d] OFF\n", idx);
                    midi_event_clear(
                        &g->m_seq.patterns[g->m_seq.cur_pattern][idx]);
                    looper_stop(g);
                }
                g->audio_buffer_read_idx =
                    new_read_idx + (g->step_diff * g->size_of_sixteenth);
            }
        }
        break;

    case (TIME_SIXTEENTH_TICK):
        if (mixr->timing_info.sixteenth_note_tick != g->m_seq.sixteenth_tick)
        {
            // printf("nOTE OFF!\n");
        }
        step_tick(&g->m_seq);

        if (g->scramble_mode)
        {
            g->scramble_diff = 0;
            int cur_sixteenth = mixr->timing_info.sixteenth_note_tick % 16;
            if (cur_sixteenth % 2 != 0)
            {
                int randy = rand() % 100;
                if (randy < 25) // repeat the third 16th
                    g->scramble_diff = 3 - cur_sixteenth;
                else if (randy > 25 && randy < 50) // repeat the 4th sixteenth
                    g->scramble_diff = 4 - cur_sixteenth;
                else if (randy > 25 && randy < 50) // repeat the 7th sixteenth
                    g->scramble_diff = 7 - cur_sixteenth;
            }
        }
        if (g->stutter_mode)
        {
            if (rand() % 100 > 75)
                g->stutter_idx++;
            if (g->stutter_idx == 16)
                g->stutter_idx = 0;
        }
        break;
    }
}

void looper_update_lfos(looper *g)
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
        // g->grain_duration_ms = scaley_val;
        looper_set_grain_duration(g, scaley_val);
    }

    if (g->grainps_lfo_on)
    {
        double lfo2_out = lfo_do_oscillate((oscillator *)&g->m_lfo2, NULL);
        double scaley_val =
            scaleybum(-1, 1, g->m_lfo2_min, g->m_lfo2_max, lfo2_out);
        // g->grains_per_sec = scaley_val;
        looper_set_grain_density(g, scaley_val);
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

stereo_val looper_gennext(void *self)
{
    looper *g = (looper *)self;
    stereo_val val = {0., 0.};

    if (!g->started || !g->sound_generator.active)
        return val;

    if (g->m_eg1.m_state == OFFF)
        g->sound_generator.active = false;

    if (g->external_source_sg != -1 && !g->buffer_is_full)
    {
        if (g->should_start_recording)
        {
            if (mixer_is_valid_soundgen_num(mixr, g->external_source_sg))
            {
                g->audio_buffer[g->audio_buffer_write_idx] =
                    mixr->soundgen_cur_val[g->external_source_sg].left;
                g->audio_buffer[g->audio_buffer_write_idx + 1] =
                    mixr->soundgen_cur_val[g->external_source_sg].right;
                g->audio_buffer_write_idx = g->audio_buffer_write_idx + 2;
                if (g->audio_buffer_write_idx >= g->audio_buffer_len)
                {
                    g->audio_buffer_write_idx = 0;
                    g->buffer_is_full = true;
                }
            }
        }
    }

    looper_update_lfos(g);
    if (g->have_active_buffer) // file buffer or external in
    {
        // STEP 1 - calculate if we should launch a new grain
        int spacing = looper_calculate_grain_spacing(g);
        if (mixr->timing_info.cur_sample >
            g->last_grain_launched_sample_time + spacing) // new grain time
        {
            g->last_grain_launched_sample_time = mixr->timing_info.cur_sample;
            g->cur_grain_num = looper_get_available_grain_num(g);

            int duration = g->grain_duration_ms * 44.1;
            if (g->quasi_grain_fudge != 0)
                duration += rand() % (int)(g->quasi_grain_fudge * 44.1);

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
            g->num_active_grains = looper_count_active_grains(g);
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

void looper_status(void *self, wchar_t *status_string)
{
    looper *g = (looper *)self;
    char *INSTRUMENT_COLOR = ANSI_COLOR_RESET;
    if (g->sound_generator.active)
        INSTRUMENT_COLOR = ANSI_COLOR_RED;

    swprintf(
        status_string, MAX_STATIC_STRING_SZ,
        WANSI_COLOR_WHITE
        "source:%s %s vol:%.2lf pitch:%.2f\n"
        "loop_mode:%s gate_mode:%d idx:%.0f buf_len:%d\n"
        "loop_len:%.2f scramble:%d stutter:%d step:%d reverse:%d "
        "buffer_is_full:%d\n"
        "gen_src:%d gen_every_n:%d gen_en:%d gen mode:%d extsource:%d\n"
        "grain_dur_ms:%d grains_per_sec:%d density_dur_sync:%d "
        "quasi_grain_fudge:%d\n"
        "fill_factor:%.2f grain_spray_ms:%.2f selection_mode:%d env_mode:%s\n"

        "[Grain Pitch LFO]\n"
        "gp_on:%d gp_type:%d gp_amp:%.2f gp_rate:%.2f gp_lo:%.2f gp_hi:%.2f\n"

        "[Grain Duration LFO]\n"
        "gd_on:%d gd_type:%d gd_amp:%.2f gd_rate:%.2f gd_lo:%.2f gd_hi:%.2f\n"

        "[Grains Per Sec LFO]\n"
        "gps_on:%d gps_type:%d gps_amp:%.2f gps_rate:%.2f gps_lo:%.2f "
        "gps_hi:%.2f\n"

        "[Grain Scan LFO]\n"
        "gs_on:%d gs_type:%d gs_amp:%.2f gs_rate:%.2f gs_lo:%.2f gs_hi:%.2f\n"

        "[Envelope Generator]\n"
        "eg_attack_ms:%.2f sustain_ms:%d eg_release_ms:%.2f eg_state:%d",

        g->filename, INSTRUMENT_COLOR, g->vol, g->grain_pitch,
        s_loop_mode_names[g->loop_mode], g->gate_mode, g->audio_buffer_read_idx,
        g->audio_buffer_len, g->loop_len, g->scramble_mode, g->stutter_mode,
        g->step_mode, g->reverse_mode, g->buffer_is_full, g->m_seq.generate_src,
        g->m_seq.generate_every_n_loops, g->m_seq.generate_en,
        g->m_seq.generate_mode, g->external_source_sg,

        g->grain_duration_ms, g->grains_per_sec, g->density_duration_sync,
        g->quasi_grain_fudge, g->fill_factor, g->granular_spray_frames / 44.1,
        g->selection_mode, s_env_names[g->envelope_mode],

        g->grainpitch_lfo_on, g->m_lfo4.osc.m_waveform,
        g->m_lfo4.osc.m_amplitude, g->m_lfo4.osc.m_osc_fo, g->m_lfo4_min,
        g->m_lfo4_max,

        g->graindur_lfo_on, g->m_lfo1.osc.m_waveform, g->m_lfo1.osc.m_amplitude,
        g->m_lfo1.osc.m_osc_fo, g->m_lfo1_min, g->m_lfo1_max,

        g->grainps_lfo_on, g->m_lfo2.osc.m_waveform, g->m_lfo2.osc.m_amplitude,
        g->m_lfo2.osc.m_osc_fo, g->m_lfo2_min, g->m_lfo2_max,

        g->grainscanfile_lfo_on, g->m_lfo3.osc.m_waveform,
        g->m_lfo3.osc.m_amplitude, g->m_lfo3.osc.m_osc_fo, g->m_lfo3_min,
        g->m_lfo3_max,

        g->m_eg1.m_attack_time_msec, g->sustain_ms,
        g->m_eg1.m_release_time_msec, g->m_eg1.m_state);

    wchar_t local_status_string[MAX_STATIC_STRING_SZ] = {};
    step_status(&g->m_seq, local_status_string);
    wcscat(status_string, local_status_string);

    wcscat(status_string, WANSI_COLOR_RESET);
}

void looper_start(void *self)
{
    looper *g = (looper *)self;
    eg_start_eg(&g->m_eg1);
    g->sound_generator.active = true;
    // g->started = false;
}

void looper_stop(void *self)
{
    looper *g = (looper *)self;
    eg_release(&g->m_eg1);
}

double looper_getvol(void *self)
{
    looper *g = (looper *)self;
    return g->vol;
}

void looper_setvol(void *self, double v)
{
    looper *g = (looper *)self;
    if (v < 0.0 || v > 1.0)
    {
        return;
    }
    g->vol = v;
}

void looper_del_self(void *self)
{
    // TODO delete file
    looper *g = (looper *)self;
    free(g);
}

void looper_make_active_track(void *self, int track_num)
{
    // NOOP
    (void)self;
    (void)track_num;
}

int looper_get_num_patterns(void *self)
{
    (void)self;
    return 1;
}

void looper_set_num_patterns(void *self, int num_patterns)
{
    (void)self;
    (void)num_patterns;
}

//////////////////////////// grain stuff //////////////////////////
// looper functions contuine below

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
        // sound_grain_check_idx(&read_idx_right, audio_buffer_len);
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
    int len = g->grain_len_frames;
    double percent_pos = 0;
    int relative_position = 0;
    double a = 0.5;

    switch (envelope_mode)
    {
    case (LOOPER_ENV_PARABOLIC):
        g->amp = g->amp + g->slope;
        g->slope = g->slope + g->curve;
        amp = g->amp;
        break;
    case (LOOPER_ENV_TRAPEZOIDAL):
        amp = 1;
        percent_pos =
            100. / g->grain_len_frames * (idx - g->audiobuffer_start_idx);
        if (percent_pos < g->attack_time_pct)
            amp *= percent_pos / g->attack_time_pct;
        else if (percent_pos > (100 - g->release_time_pct))
            amp *= (100 - percent_pos) / g->release_time_pct;
        break;
    case (LOOPER_ENV_TUKEY_WINDOW):
        relative_position = g->audiobuffer_cur_pos - g->audiobuffer_start_idx;
        amp = 0.5 * (1 + cos(M_PI * ((relative_position * 2 /
                                      (0.5 * (g->grain_len_frames - 1))) -
                                     1)));
        break;
    }

    return amp;
}

//////////////////////////// end of grain stuff //////////////////////////

void looper_import_file(looper *g, char *filename)
{
    strncpy(g->filename, filename, 512);
    audio_buffer_details deetz =
        import_file_contents(&g->audio_buffer, filename);
    g->audio_buffer_len = deetz.buffer_length;
    g->num_channels = deetz.num_channels;
    g->external_source_sg = -1;
    g->have_active_buffer = true;
    g->size_of_sixteenth = g->audio_buffer_len / 16;
}

void looper_set_external_source(looper *g, int sound_gen_num)
{
    if (mixer_is_valid_soundgen_num(mixr, sound_gen_num))
    {
        g->external_source_sg = sound_gen_num;
        int looplen = mixr->timing_info.loop_len_in_frames * 2; // stereo
        double *buffer = calloc(looplen, sizeof(double));
        if (buffer)
        {
            if (g->audio_buffer)
            {
                free(g->audio_buffer);
                g->buffer_is_full = false;
            }

            g->audio_buffer = buffer;
            g->audio_buffer_len = looplen;
            g->num_channels = 2;
            g->have_active_buffer = true;
            g->size_of_sixteenth = g->audio_buffer_len / 16;
            g->should_start_recording = false; // reset
        }
    }
}

int looper_calculate_grain_spacing(looper *g)
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

void looper_set_grain_duration(looper *l, int dur)
{
    l->grain_duration_ms = dur;
    if (l->density_duration_sync)
        l->grains_per_sec = (1000. / l->grain_duration_ms) * l->fill_factor;
}

void looper_set_grain_density(looper *l, int gps)
{
    l->grains_per_sec = gps;
    if (l->density_duration_sync)
        l->grain_duration_ms = (l->fill_factor / l->grains_per_sec) * 1000;
}

void looper_set_grain_attack_size_pct(looper *g, int attack_pct)
{
    if (attack_pct < 50)
        g->grain_attack_time_pct = attack_pct;
}

void looper_set_grain_release_size_pct(looper *g, int release_pct)
{
    if (release_pct < 50)
        g->grain_release_time_pct = release_pct;
}

void looper_set_audio_buffer_read_idx(looper *g, int pos)
{
    if (pos < 0 || pos >= g->audio_buffer_len)
    {
        return;
    }
    g->audio_buffer_read_idx = pos;
}

void looper_set_granular_spray(looper *g, int spray_ms)
{
    int spray_frames = spray_ms * 44.1;
    g->granular_spray_frames = spray_frames;
}

void looper_set_quasi_grain_fudge(looper *g, int fudgefactor)
{
    g->quasi_grain_fudge = fudgefactor;
}

void looper_set_grain_pitch(looper *g, double pitch) { g->grain_pitch = pitch; }

void looper_set_selection_mode(looper *g, unsigned int mode)
{
    if (mode >= GRAIN_NUM_SELECTION_MODES)
    {
        printf("Selection must be < %d\n", GRAIN_NUM_SELECTION_MODES);
        return;
    }
    g->selection_mode = mode;
}

void looper_set_envelope_mode(looper *g, unsigned int mode)
{
    if (mode >= LOOPER_ENV_NUM)
    {
        printf("Selection must be < %d\n", LOOPER_ENV_NUM);
        return;
    }
    g->envelope_mode = mode;
}

void looper_set_reverse_mode(looper *g, bool b) { g->reverse_mode = b; }
void looper_set_loop_mode(looper *g, unsigned int m)
{
    g->loop_mode = m;
    if (m == LOOPER_STATIC_MODE)
    {
        g->quasi_grain_fudge = 220;
        g->selection_mode = GRAIN_SELECTION_STATIC;
        g->granular_spray_frames = 441; // 10ms * (44100/1000)
        g->grain_duration_ms = 50;
    }
    else if (m == LOOPER_LOOP_MODE)
    {
        g->quasi_grain_fudge = 0;
        g->selection_mode = GRAIN_SELECTION_STATIC;
        g->granular_spray_frames = 0;
        g->grain_duration_ms = 100;
    }
}
void looper_set_scramble_pending(looper *g) { g->scramble_pending = true; }

void looper_set_stutter_pending(looper *g) { g->stutter_pending = true; }

void looper_set_step_pending(looper *g) { g->step_pending = true; }

void looper_set_loop_len(looper *g, double bars)
{
    if (bars != 0)
        g->loop_len = bars;
}

int looper_get_available_grain_num(looper *g)
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
    printf("WOW - NO GRAINS TO BE FOUND IN %d attempts\n", idx);
    return 0;
}

int looper_count_active_grains(looper *g)
{
    int active = 0;
    for (int i = 0; i < g->highest_grain_num; i++)
        if (g->m_grains[i].active)
            active++;

    return active;
}

void looper_set_lfo_amp(looper *g, int lfonum, double amp)
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

void looper_set_lfo_voice(looper *g, int lfonum, unsigned int voice)
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

void looper_set_lfo_rate(looper *g, int lfonum, double rate)
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

void looper_set_lfo_min(looper *g, int lfonum, double minval)
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

void looper_set_lfo_max(looper *g, int lfonum, double maxval)
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

void looper_set_lfo_sync(looper *g, int lfonum, int numloops)
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

void looper_set_fill_factor(looper *l, double fill_factor)
{
    if (fill_factor >= 0. && fill_factor <= 2.)
        l->fill_factor = fill_factor;
}

void looper_set_density_duration_sync(looper *l, bool b)
{
    l->density_duration_sync = b;
}

void looper_dump_buffer(looper *l)
{
    printf("Buffer is full:%d\n", l->buffer_is_full);
    printf("Buffer Len:%d\n", l->audio_buffer_len);
    printf("Rdx Idx:%f Write Idx:%d\n", l->audio_buffer_read_idx,
           l->audio_buffer_write_idx);
    printf("Num active grains:%d\n", l->num_active_grains);
    // for (int i = 0; i < l->audio_buffer_len; i+=2)
    //    printf("Left:%f Right:%f\n", l->audio_buffer[i],
    //    l->audio_buffer[i+1]);
    for (int i = 0; i < l->highest_grain_num; i++)
    {
        sound_grain *g = &l->m_grains[i];
        printf("Grain:%d len:%d, buf_num:%d start_idx:%d cur_pos:%f inc:%f "
               "active:%d\n",
               i, g->grain_len_frames, g->audiobuffer_num,
               g->audiobuffer_start_idx, g->audiobuffer_cur_pos,
               g->audiobuffer_inc, g->active);
    }
}
void looper_set_gate_mode(looper *g, bool b) { g->gate_mode = b; }
void looper_set_sustain_ms(looper *l, int sustain_ms)
{
    if (sustain_ms >= 0)
        l->sustain_ms = sustain_ms;
}
