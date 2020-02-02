#include <iostream>
#include <libgen.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <defjams.h>
#include <looper.h>
#include <mixer.h>
#include <pattern_utils.h>
#include <utils.h>

extern mixer *mixr;
extern char *s_lfo_mode_names;

static char *s_env_names[] = {(char *)"PARABOLIC", (char *)"TRAPEZOIDAL",
                              (char *)"TUKEY", (char *)"GENERATOR"};
static char *s_loop_mode_names[] = {(char *)"LOOP", (char *)"STATIC",
                                    (char *)"SMUDGE"};
static char *s_external_mode_names[] = {(char *)"FOLLOW", (char *)"CAPTURE"};

looper::looper(char *filename)
{
    have_active_buffer = false;

    audio_buffer_read_idx = 0;
    granular_spray_frames = 441; // 10ms * (44100/1000)
    grain_attack_time_pct = 15;
    grain_release_time_pct = 15;
    quasi_grain_fudge = 220;
    selection_mode = GRAIN_SELECTION_STATIC;
    // envelope_mode = LOOPER_ENV_GENERATOR;
    envelope_mode = LOOPER_ENV_PARABOLIC;
    envelope_taper_ratio = 0.5;
    reverse_mode = 0; // bool
    external_source_sg = -1;
    external_source_mode = LOOPER_EXTERNAL_MODE_FOLLOW;
    if (pthread_mutex_init(&extsource_lock, NULL) != 0)
    {
        printf("\n mutex init has failed\n");
        return;
    }
    recording = false;

    loop_mode = LOOPER_SMUDGE_MODE;
    loop_counter = -1;
    scramble_mode = false;
    stutter_mode = false;
    stutter_idx = 0;

    grain_pitch = 1;

    density_duration_sync = true;
    fill_factor = 3.;
    looper_set_grain_density(this, 30);

    type = LOOPER_TYPE;

    if (strncmp(filename, "none", 4) != 0)
        looper_import_file(this, filename);

    engine.sustain_note_ms = 500;

    envelope_generator_init(&m_eg1); // start/stop env
    m_eg1.m_attack_time_msec = 10;
    m_eg1.m_release_time_msec = 50;

    degrade_by = 0;
    gate_mode = false;
    start();
}

void looper::eventNotify(broadcast_event event, mixer_timing_info tinfo)
{
    // need to read cur_step before calling SoundGenerator::eventNotify
    int cur_sixteenth_midi_base = engine.cur_step * PPSIXTEENTH;
    if (cur_sixteenth_midi_base < 0)
        cur_sixteenth_midi_base = 0;
    int cur_midi_idx =
        (cur_sixteenth_midi_base + (mixr->timing_info.midi_tick % PPSIXTEENTH));

    SoundGenerator::eventNotify(event, tinfo);

    if (!engine.started)
        return;
    started = true;

    if (tinfo.is_start_of_loop)
    {
        loop_counter++;

        if (scramble_pending)
        {
            scramble_mode = true;
            scramble_pending = false;
        }
        else
            scramble_mode = false;

        if (stutter_pending)
        {
            stutter_mode = true;
            stutter_pending = false;
        }
        else
            stutter_mode = false;

        if (record_pending)
        {
            recording = true;
            record_pending = false;
        }
    }

    // used to track which 16th we're on if loop != 1 bar
    float loop_num = fmod(loop_counter, loop_len);
    if (loop_num < 0)
        loop_num = 0;

    int relative_midi_idx = (loop_num * PPBAR) + cur_midi_idx;
    double decimal_percent_of_loop = relative_midi_idx / (PPBAR * loop_len);
    normalized_audio_buffer_read_idx =
        decimal_percent_of_loop * audio_buffer_len;

    if (loop_mode == LOOPER_LOOP_MODE)
    {
        // used to track which 16th we're on if loop != 1 bar
        float loop_num = fmod(loop_counter, loop_len);
        if (loop_num < 0)
            loop_num = 0;

        int relative_midi_idx = (loop_num * PPBAR) + cur_midi_idx;
        double decimal_percent_of_loop = relative_midi_idx / (PPBAR * loop_len);
        double new_read_idx = decimal_percent_of_loop * audio_buffer_len;
        if (reverse_mode)
            new_read_idx = (audio_buffer_len - 1) - new_read_idx;

        // this ensures new_read_idx is even
        if (num_channels == 2)
            new_read_idx -= ((int)new_read_idx & 1);

        if (scramble_mode)
        {
            audio_buffer_read_idx =
                new_read_idx + (scramble_diff * size_of_sixteenth);
        }
        else if (stutter_mode)
        {
            int rel_pos_within_a_sixteenth =
                new_read_idx - (engine.cur_step * size_of_sixteenth);
            audio_buffer_read_idx =
                (stutter_idx * size_of_sixteenth) + rel_pos_within_a_sixteenth;
        }
        else
            audio_buffer_read_idx = new_read_idx;
    }

    // step sequencer as env generator
    // if (engine.patterns[engine.cur_pattern][cur_midi_idx].event_type ==
    // MIDI_ON)
    //{
    //    eg_start_eg(&m_eg1);

    //    midi_event *pattern = engine.patterns[engine.cur_pattern];
    //    int off_tick =
    //        (int)(cur_midi_idx +
    //              ((m_eg1.m_attack_time_msec + m_eg1.m_decay_time_msec +
    //                engine.sustain_note_ms) *
    //               mixr->timing_info.ms_per_midi_tick)) %
    //        PPBAR;
    //    midi_event off_event = new_midi_event(MIDI_OFF, 0, 128);
    //    midi_pattern_add_event(pattern, off_tick, off_event);
    //}
    // else if (engine.patterns[engine.cur_pattern][cur_midi_idx].event_type ==
    //         MIDI_OFF)
    //{
    //    // printf("[%d] OFF\n", idx);
    //    midi_event_clear(&engine.patterns[engine.cur_pattern][cur_midi_idx]);
    //    m_eg1.m_state = RELEASE;
    //}

    // printf(
    //    "LOOP NUM:%f cur_sixteenth:%d cur_16th_midi_base:%d
    //    midi_idx:%d\n", loop_num, engine.cur_step,
    //    cur_sixteenth_midi_base, cur_midi_idx);

    if (tinfo.is_sixteenth)
    {
        if (scramble_mode)
        {
            scramble_diff = 0;
            int cur_sixteenth = mixr->timing_info.sixteenth_note_tick % 16;
            if (cur_sixteenth % 2 != 0)
            {
                int randy = rand() % 100;
                if (randy < 25) // repeat the third 16th
                    scramble_diff = 3 - cur_sixteenth;
                else if (randy > 25 && randy < 50) // repeat the 4th sixteenth
                    scramble_diff = 4 - cur_sixteenth;
                else if (randy > 50 && randy < 75) // repeat the 7th sixteenth
                    scramble_diff = 7 - cur_sixteenth;
            }
        }
        if (stutter_mode)
        {
            if (rand() % 100 > 75)
                stutter_idx++;
            if (stutter_idx == 16)
                stutter_idx = 0;
        }
    }
}

stereo_val looper::genNext()
{
    stereo_val val = {0., 0.};

    if (!started || !active)
        return val;

    if (stop_pending && m_eg1.m_state == OFFF)
        active = false;

    if (external_source_sg != -1)
    {
        if (recording)
        {
            if (mixer_is_valid_soundgen_num(mixr, external_source_sg))
            {
                audio_buffer[audio_buffer_write_idx] =
                    mixr->soundgen_cur_val[external_source_sg].left;
                audio_buffer[audio_buffer_write_idx + 1] =
                    mixr->soundgen_cur_val[external_source_sg].right;
                audio_buffer_write_idx = audio_buffer_write_idx + 2;

                if (audio_buffer_write_idx >= audio_buffer_len)
                {
                    audio_buffer_write_idx = 0;

                    if (external_source_mode ==
                        LOOPER_EXTERNAL_MODE_CAPTURE_ONCE)
                        recording = false;
                }
            }
        }
    }

    if (have_active_buffer) // file buffer or external in
    {
        if (pthread_mutex_trylock(&extsource_lock) == 0)
        {
            // STEP 1 - calculate if we should launch a new grain
            int spacing = looper_calculate_grain_spacing(this);
            if (mixr->timing_info.cur_sample >
                last_grain_launched_sample_time + spacing) // new grain time
            {
                last_grain_launched_sample_time = mixr->timing_info.cur_sample;
                cur_grain_num = looper_get_available_grain_num(this);

                int duration = grain_duration_ms * 44.1;
                if (quasi_grain_fudge != 0)
                    duration += rand() % (int)(quasi_grain_fudge * 44.1);

                int grain_idx = audio_buffer_read_idx;
                if (selection_mode == GRAIN_SELECTION_RANDOM)
                    grain_idx =
                        rand() % (audio_buffer_len - (duration * num_channels));

                if (granular_spray_frames > 0)
                    grain_idx += rand() % granular_spray_frames;

                int attack_time_pct = grain_attack_time_pct;
                int release_time_pct = grain_release_time_pct;

                bool enable_debug = false;
                if (debug_pending)
                {
                    enable_debug = true;
                    debug_pending = false;
                }
                sound_grain_params params = {.dur = duration,
                                             .starting_idx = grain_idx,
                                             .attack_pct = attack_time_pct,
                                             .release_pct = release_time_pct,
                                             .reverse_mode = reverse_mode,
                                             .pitch = grain_pitch,
                                             .num_channels = num_channels,
                                             .degrade_by = degrade_by,
                                             .debug = enable_debug};

                sound_grain_init(&m_grains[cur_grain_num], params);
                num_active_grains = looper_count_active_grains(this);
            }

            // STEP 2 - gather vals from all active grains
            for (int i = 0; i < highest_grain_num; i++)
            // for (int i = 0; i < 1 && i < highest_grain_num; i++)
            {
                sound_grain *sgr = &m_grains[i];
                stereo_val tmp =
                    sound_grain_generate(sgr, audio_buffer, audio_buffer_len);
                double env = sound_grain_env(sgr, envelope_mode);

                val.left += tmp.left * env;
                val.right += tmp.right * env;
            }
            if (pthread_mutex_unlock(&extsource_lock) != 0)
                printf("Youch, couldn't unlock mutex!\n");
        }
    }

    eg_update(&m_eg1);
    double eg_amp = eg_do_envelope(&m_eg1, NULL);

    pan = fmin(pan, 1.0);
    pan = fmax(pan, -1.0);
    double pan_left = 0.707;
    double pan_right = 0.707;
    calculate_pan_values(pan, &pan_left, &pan_right);

    val.left = val.left * volume * eg_amp * pan_left;
    val.right = val.right * volume * eg_amp * pan_right;

    val = Effector(val);

    return val;
}

std::string looper::Status()
{
    std::stringstream ss;
    ss << ANSI_COLOR_RED << "Granular(" << filename << ")"
       << " vol:" << volume << " pan:" << pan << " pitch:" << grain_pitch
       << " idx:" << (int)(100. / audio_buffer_len * audio_buffer_read_idx)
       << " mode:" << s_loop_mode_names[loop_mode] << "(" << loop_mode << ")"
       << ANSI_COLOR_RESET;
    return ss.str();
}
std::string looper::Info()
{
    char *INSTRUMENT_COLOR = (char *)ANSI_COLOR_RESET;
    if (active)
        INSTRUMENT_COLOR = (char *)ANSI_COLOR_RED;

    std::stringstream ss;
    ss << ANSI_COLOR_WHITE << filename << INSTRUMENT_COLOR << " vol:" << volume
       << " pan:" << pan << " pitch:" << grain_pitch
       << " mode:" << s_loop_mode_names[loop_mode] << "\n";

    //        %d mode:%s\n"
    //"gate_mode:%d idx:%.0f buf_len:%d atk:%d rel:%d\n"
    //"len:%.2f scramble:%d stutter:%d step:%d reverse:%d\n"
    //"xsrc:%d rec:%d widx:%d xmode:%s(%d) degrade:%d\n "
    //"grain_dur_ms:" "%s" "%d" "%s "
    //"grains_per_sec:" "%s" "%d" "%s "
    //"density_dur_sync:%d "
    //"quasi_grain_fudge:%d\n"
    //"fill_factor:%.2f grain_spray_ms:%.2f selection_mode:%d env_mode:%s\n"

    //"[" "%s" "Envelope Generator" "%s" "]\n"
    //"eg_attack_ms:%.2f eg_release_ms:%.2f eg_state:%d",
    //// clang-format on

    // num_channels > 1 ? 1 : 0, s_loop_mode_names[loop_mode], gate_mode,
    // audio_buffer_read_idx, audio_buffer_len, grain_attack_time_pct,
    // grain_release_time_pct, loop_len, scramble_mode, stutter_mode,
    // step_mode, reverse_mode, external_source_sg, recording,
    // audio_buffer_write_idx, s_external_mode_names[external_source_mode],
    // external_source_mode, degrade_by,

    // ANSI_COLOR_WHITE, grain_duration_ms, INSTRUMENT_COLOR, ANSI_COLOR_WHITE,
    // grains_per_sec, INSTRUMENT_COLOR, density_duration_sync,
    // quasi_grain_fudge, fill_factor, granular_spray_frames / 44.1,
    // selection_mode, s_env_names[envelope_mode],

    // ANSI_COLOR_WHITE, INSTRUMENT_COLOR, m_eg1.m_attack_time_msec,
    // m_eg1.m_release_time_msec, m_eg1.m_state);

    return ss.str();
    // wchar_t local_status_string[MAX_STATIC_STRING_SZ] = {};
    // sequence_engine_status(&engine, local_status_string);
    // wcscat(status_string, local_status_string);

    // wcscat(status_string, WANSI_COLOR_RESET);
}

void looper::start()
{
    eg_start_eg(&m_eg1);
    active = true;
    stop_pending = false;
    sequence_engine_reset(&engine);
}

void looper::stop()
{
    eg_release(&m_eg1);
    stop_pending = true;
}

looper::~looper()
{
    // TODO delete file
}

//////////////////////////// grain stuff //////////////////////////
// looper functions continue below

void sound_grain_init(sound_grain *g, sound_grain_params params)
{
    g->audiobuffer_start_idx = params.starting_idx;
    g->grain_len_frames = params.dur;
    g->grain_counter_frames = 0;
    g->attack_time_pct = params.attack_pct;
    g->release_time_pct = params.release_pct;
    g->audiobuffer_num_channels = params.num_channels;
    g->degrade_by = params.degrade_by;
    g->debug = params.debug;

    g->reverse_mode = params.reverse_mode;
    if (params.reverse_mode)
    {
        g->audiobuffer_cur_pos =
            params.starting_idx + (params.dur * params.num_channels) - 1;
        g->incr = -1.0 * params.pitch;
    }
    else
    {
        g->audiobuffer_cur_pos = params.starting_idx;
        g->incr = params.pitch;
    }

    g->attack_time_samples = params.dur / 100. * params.attack_pct;
    g->release_time_samples = params.dur / 100. * params.release_pct;

    g->amp = 0;
    double rdur = 1.0 / params.dur;
    double rdur2 = rdur * rdur;
    g->slope = 4.0 * 1.0 * (rdur - rdur2);
    g->curve = -8.0 * 1.0 * rdur2;

    double loop_len_ms = 1000. * params.dur / SAMPLE_RATE;
    double attack_time_ms = loop_len_ms / 100. * params.attack_pct;
    double release_time_ms = loop_len_ms / 100. * params.release_pct;
    // printf("ATTACKMS: %f RELEASEMS: %f\n", attack_time_ms, release_time_ms);

    envelope_generator_init(&g->eg);
    eg_set_attack_time_msec(&g->eg, attack_time_ms);
    eg_set_decay_time_msec(&g->eg, 0);
    eg_set_release_time_msec(&g->eg, release_time_ms);
    eg_start_eg(&g->eg);

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

    if (g->degrade_by > 0)
    {
        if (rand() % 100 < g->degrade_by)
            return out;
    }

    int num_channels = g->audiobuffer_num_channels;

    int read_idx = (int)g->audiobuffer_cur_pos;
    double frac = g->audiobuffer_cur_pos - read_idx;
    sound_grain_check_idx(&read_idx, audio_buffer_len);

    if (num_channels == 1)
    {
        int read_next_idx = read_idx + 1;
        sound_grain_check_idx(&read_next_idx, audio_buffer_len);
        out.left = lin_terp(0, 1, audio_buffer[read_idx],
                            audio_buffer[read_next_idx], frac);
        out.right = out.left;
    }
    else if (num_channels == 2)
    {
        int read_next_idx = read_idx + 2;
        sound_grain_check_idx(&read_next_idx, audio_buffer_len);
        out.left = lin_terp(0, 1, audio_buffer[read_idx],
                            audio_buffer[read_next_idx], frac);

        int read_idx_right = read_idx + 1;
        sound_grain_check_idx(&read_idx_right, audio_buffer_len);
        int read_next_idx_right = read_idx_right + 2;
        sound_grain_check_idx(&read_next_idx_right, audio_buffer_len);
        out.right = lin_terp(0, 1, audio_buffer[read_idx_right],
                             audio_buffer[read_next_idx_right], frac);
    }

    g->audiobuffer_cur_pos += (g->incr * num_channels);

    g->grain_counter_frames++;
    if (g->grain_counter_frames > g->grain_len_frames)
    {
        g->active = false;
    }

    return out;
}

double sound_grain_env(sound_grain *g, unsigned int envelope_mode)
{
    if (!g->active)
        return 0.;

    double amp = 1;
    double one_percent = g->grain_len_frames / 100.0;
    double percent_pos = g->grain_counter_frames / one_percent;

    switch (envelope_mode)
    {
    case (LOOPER_ENV_PARABOLIC):
        g->amp = g->amp + g->slope;
        g->slope = g->slope + g->curve;
        amp = g->amp;
        break;
    case (LOOPER_ENV_TRAPEZOIDAL):
        if (percent_pos < g->attack_time_pct)
            amp *= (percent_pos / g->attack_time_pct);
        else if (percent_pos > (100 - g->release_time_pct))
            amp *= (100 - percent_pos) / g->release_time_pct;
        break;
    case (LOOPER_ENV_TUKEY_WINDOW):
        // amp = 0.5 * (1 - cos(2 * M_PI * g->grain_counter_frames /
        //                     g->attack_time_samples));
        if (percent_pos < g->attack_time_pct)
        {
            amp = (1.0 + cos(M_PI + (M_PI *
                                     ((double)g->grain_counter_frames /
                                      g->attack_time_samples) *
                                     (1.0 / 2.0))));
        }
        else if (percent_pos > (100 - g->release_time_pct))
        {
            double attack_and_sustain_len_frames =
                g->grain_len_frames - g->release_time_samples;

            amp =
                cos(M_PI *
                    ((g->grain_counter_frames - attack_and_sustain_len_frames) /
                     g->release_time_samples) *
                    (1.0 / 2.0));
        }
        break;
    case (LOOPER_ENV_GENERATOR):
        amp = eg_do_envelope(&g->eg, NULL);
        // printf("PCT:%f FRAME:%d AMP is %f\n", percent_pos,
        //      g->grain_counter_frames, amp);
        if (percent_pos > (100 - g->release_time_pct))
        {
            eg_note_off(&g->eg);
        }
        break;
    }

    if (g->debug)
        printf("%f\n", amp);

    return amp;
}

//////////////////////////// end of grain stuff //////////////////////////

void looper_import_file(looper *g, char *filename)
{
    strncpy(g->filename, filename, 512);
    if (pthread_mutex_lock(&g->extsource_lock) == 0)
    {
        audio_buffer_details deetz =
            import_file_contents(&g->audio_buffer, filename);
        g->audio_buffer_len = deetz.buffer_length;
        g->num_channels = deetz.num_channels;
        g->external_source_sg = -1;
        g->have_active_buffer = true;
        looper_set_loop_len(g, 1);
    }
    else
    {
        printf("Couldn't lock mutex for buffer access!\n");
    }
    if (pthread_mutex_unlock(&g->extsource_lock) != 0)
        printf("Youch, couldn't unlock mutex!\n");
}

void looper_set_external_source(looper *g, int sound_gen_num)
{
    if (mixer_is_valid_soundgen_num(mixr, sound_gen_num))
    {
        g->external_source_sg = sound_gen_num;
        int looplen = mixr->timing_info.loop_len_in_frames * 2; // stereo
        double *buffer = (double *)calloc(looplen, sizeof(double));
        if (buffer)
        {
            if (pthread_mutex_lock(&g->extsource_lock) == 0)
            {
                if (g->audio_buffer)
                    free(g->audio_buffer);

                g->audio_buffer = buffer;
                g->audio_buffer_len = looplen;
                g->num_channels = 2;
                g->have_active_buffer = true;
                g->size_of_sixteenth = g->audio_buffer_len / 16;
                g->record_pending = true; // reset
            }
            else
            {
                printf("Couldn't lock buffer!\n");
                free(buffer);
            }
            if (pthread_mutex_unlock(&g->extsource_lock) != 0)
                printf("Youch, couldn't unlock mutex!\n");
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
        l->grains_per_sec = 1000. / (l->grain_duration_ms / l->fill_factor);
}

void looper_set_grain_density(looper *l, int gps)
{
    l->grains_per_sec = gps;
    if (l->density_duration_sync)
        l->grain_duration_ms = 1000. / l->grains_per_sec * l->fill_factor;
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
    g->selection_mode = GRAIN_SELECTION_STATIC;
    if (m == LOOPER_SMUDGE_MODE)
    {
        g->quasi_grain_fudge = 220;
        g->granular_spray_frames = 441; // 10ms * (44100/1000)
    }
    else
    {
        g->quasi_grain_fudge = 0;
        g->granular_spray_frames = 0;
    }
}
void looper_set_scramble_pending(looper *g) { g->scramble_pending = true; }

void looper_set_stutter_pending(looper *g) { g->stutter_pending = true; }

void looper_set_step_mode(looper *g, bool b) { g->step_mode = b; }

void looper_set_loop_len(looper *g, double bars)
{
    if (bars != 0)
    {
        g->loop_len = bars;
        g->size_of_sixteenth = g->audio_buffer_len / 16 * bars;
    }
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

void looper_set_fill_factor(looper *l, double fill_factor)
{
    if (fill_factor >= 0. && fill_factor <= 10.)
        l->fill_factor = fill_factor;
}

void looper_set_density_duration_sync(looper *l, bool b)
{
    l->density_duration_sync = b;
}

void looper_dump_buffer(looper *l)
{
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
        printf("Grain:%d len:%d, buf_num:%d start_idx:%d cur_pos:%f incr:%f "
               "active:%d\n",
               i, g->grain_len_frames, g->audiobuffer_num,
               g->audiobuffer_start_idx, g->audiobuffer_cur_pos, g->incr,
               g->active);
    }
}
void looper_set_gate_mode(looper *g, bool b) { g->gate_mode = b; }

void looper_set_grain_env_attack_pct(looper *l, int percent)
{
    if (percent > 0 && percent < 100)
        l->grain_attack_time_pct = percent;
}
void looper_set_grain_env_release_pct(looper *l, int percent)
{
    if (percent > 0 && percent < 100)
        l->grain_release_time_pct = percent;
}

void looper_set_grain_external_source_mode(looper *l, unsigned int mode)
{
    if (mode < LOOPER_EXTERNAL_MODE_NUM)
        l->external_source_mode = mode;
}

void looper_set_degrade_by(looper *l, int degradation)
{
    if (degradation >= 0 && degradation <= 100)
        l->degrade_by = degradation;
}

void looper_set_trace_envelope(looper *l) { l->debug_pending = true; }

void looper::noteOn(midi_event ev)
{
    (void)ev;
    audio_buffer_read_idx = normalized_audio_buffer_read_idx;
}
void looper::SetParam(std::string name, double val)
{
    if (name == "pitch")
        looper_set_grain_pitch(this, val);
    else if (name == "mode")
        looper_set_loop_mode(this, val);
    else if (name == "gate_mode")
        looper_set_gate_mode(this, val);
    else if (name == "idx")
    {
        if (val <= 100)
        {
            double pos = audio_buffer_len / 100 * val;
            looper_set_audio_buffer_read_idx(this, pos);
        }
    }
    else if (name == "len")
        looper_set_loop_len(this, val);
    else if (name == "scamble")
        looper_set_scramble_pending(this);
    else if (name == "stutter")
        looper_set_stutter_pending(this);
    else if (name == "step")
        looper_set_step_mode(this, val);
    else if (name == "reverse")
        looper_set_reverse_mode(this, val);
    else if (name == "grain_dur_ms")
        looper_set_grain_duration(this, val);
    else if (name == "grains_per_sec")
        looper_set_grain_density(this, val);
    else if (name == "density_dur_sync")
        looper_set_density_duration_sync(this, val);
    else if (name == "quasi_grain_fudge")
        looper_set_quasi_grain_fudge(this, val);
    else if (name == "fill_factor")
        looper_set_fill_factor(this, val);
    else if (name == "grain_spray_ms")
        looper_set_granular_spray(this, val);
    else if (name == "selection_mode")
        looper_set_selection_mode(this, val);
    else if (name == "env_mode")
        looper_set_envelope_mode(this, val);
}

double looper::GetParam(std::string name) { return 0; }
