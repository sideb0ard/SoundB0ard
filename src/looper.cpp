#include <iostream>
#include <libgen.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <defjams.h>
#include <looper.h>
#include <mixer.h>
#include <utils.h>

extern Mixer *mixr;
extern char *s_lfo_mode_names;

static char *s_env_names[] = {(char *)"PARABOLIC", (char *)"TRAPEZOIDAL",
                              (char *)"TUKEY",     (char *)"GENERATOR",
                              (char *)"EXP_CURVE", (char *)"LOG_CURVE"};
static char *s_loop_mode_names[] = {(char *)"LOOP", (char *)"STATIC",
                                    (char *)"SMUDGE"};
static char *s_external_mode_names[] = {(char *)"FOLLOW", (char *)"CAPTURE"};

looper::looper(char *filename, bool loop_mode)
{
    std::cout << "NEW LOOOOOPPPPPER " << loop_mode << std::endl;
    have_active_buffer = false;

    audio_buffer_read_idx = 0;
    granular_spray_frames = 441; // 10ms * (44100/1000)
    grain_attack_time_pct = 15;
    grain_release_time_pct = 15;
    quasi_grain_fudge = 220;
    selection_mode = GRAIN_SELECTION_STATIC;
    // envelope_mode = LOOPER_ENV_GENERATOR;
    envelope_mode = LOOPER_ENV_LOGARITHMIC_CURVE;
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

    m_eg1.m_attack_time_msec = 10;
    m_eg1.m_release_time_msec = 50;

    degrade_by = 0;
    gate_mode = false;

    if (loop_mode)
        looper_set_loop_mode(this, LOOPER_LOOP_MODE);
    else
    {
        looper_set_loop_mode(this, LOOPER_SMUDGE_MODE);
        volume = 0.2;
    }

    start();
}

void looper::eventNotify(broadcast_event event, mixer_timing_info tinfo)
{
    // need to read cur_step before calling SoundGenerator::eventNotify
    //  int cur_sixteenth_midi_base = engine.cur_step * PPSIXTEENTH;
    //  if (cur_sixteenth_midi_base < 0)
    //      cur_sixteenth_midi_base = 0;
    //  int cur_midi_idx =
    //      (cur_sixteenth_midi_base + (mixr->timing_info.midi_tick %
    //      PPSIXTEENTH));

    SoundGenerator::eventNotify(event, tinfo);

    if (tinfo.is_midi_tick)
    {
        if (started)
        {
            // increment for next step
            cur_midi_idx_ = fmodf(cur_midi_idx_ + incr_speed_, PPBAR);
            // std::cout << "CUR+IDX:" << cur_midi_idx_ << std::endl;
        }
    }

    if (tinfo.is_start_of_loop)
    {
        // std::cout << "START OF LOOP< YO\n";
        started = true;
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

    // int relative_midi_idx = (loop_num * PPBAR) + cur_midi_idx_;
    // double decimal_percent_of_loop = relative_midi_idx / (PPBAR * loop_len);
    // normalized_audio_buffer_read_idx =
    //    decimal_percent_of_loop * audio_buffer_len;

    if (loop_mode_ == LOOPER_LOOP_MODE)
    {
        // used to track which 16th we're on if loop != 1 bar
        float loop_num = fmod(loop_counter, loop_len);
        if (loop_num < 0)
            loop_num = 0;

        int relative_midi_idx = (loop_num * PPBAR) + cur_midi_idx_;
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
    if (!engine.started)
        return;
    started = true;
}

stereo_val looper::genNext()
{
    stereo_val val = {0., 0.};

    if (!started || !active)
        return val;

    if (stop_pending && m_eg1.m_state == OFFF)
        active = false;

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
                                             .debug = enable_debug,
                                             .envelope_mode = envelope_mode};

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
                // double env = sound_grain_env(sgr, envelope_mode);

                val.left += tmp.left;
                val.right += tmp.right;
            }
            if (pthread_mutex_unlock(&extsource_lock) != 0)
                printf("Youch, couldn't unlock mutex!\n");
        }
    }

    m_eg1.Update();
    double eg_amp = m_eg1.DoEnvelope(NULL);

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
    if (!active || volume == 0)
        ss << ANSI_COLOR_RESET;
    else
        ss << ANSI_COLOR_RED;
    ss << filename << " vol:" << volume << " pan:" << pan
       << " pitch:" << grain_pitch
       << " idx:" << (int)(100. / audio_buffer_len * audio_buffer_read_idx)
       << " mode:" << s_loop_mode_names[loop_mode_] << "(" << loop_mode_ << ")"
       << " step:" << step_mode << " len:" << loop_len << ANSI_COLOR_RESET;

    return ss.str();
}
std::string looper::Info()
{
    char *INSTRUMENT_COLOR = (char *)ANSI_COLOR_RESET;
    if (active)
        INSTRUMENT_COLOR = (char *)ANSI_COLOR_RED;

    std::stringstream ss;
    ss << ANSI_COLOR_WHITE << filename << INSTRUMENT_COLOR << " vol:" << volume
       << " pan:" << pan << " pitch:" << grain_pitch << " speed:" << incr_speed_
       << " mode:" << s_loop_mode_names[loop_mode_]
       << " env_mode:" << s_env_names[envelope_mode] << "(" << envelope_mode
       << ") "
       << "\ngrain_dur_ms:" << grain_duration_ms
       << " grains_per_sec:" << grains_per_sec
       << " density_dur_sync:" << density_duration_sync
       << " quasi_grain_fudge:" << quasi_grain_fudge
       << " fill_factor:" << fill_factor
       << "\ngrain_spray_ms:" << granular_spray_frames / 44.1 << "\n";

    return ss.str();
}

void looper::start()
{
    m_eg1.StartEg();
    active = true;
    stop_pending = false;
    engine.started = false;
}

void looper::stop()
{
    m_eg1.Release();
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
    g->amp = 0;
    g->audiobuffer_start_idx = params.starting_idx;
    g->grain_len_frames = params.dur;
    g->grain_counter_frames = 0;

    g->attack_time_pct = params.attack_pct;
    g->release_time_pct = params.release_pct;

    g->attack_time_samples = params.dur / 100. * params.attack_pct;
    g->attack_to_sustain_boundary_sample_idx = g->attack_time_samples;
    g->release_time_samples = params.dur / 100. * params.release_pct;
    g->sustain_to_decay_boundary_sample_idx =
        params.dur - g->release_time_samples;

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

    switch (g->envelope_mode)
    {
    case (LOOPER_ENV_PARABOLIC):
    {
        double rdur = 1.0 / g->grain_len_frames;
        double rdur2 = rdur * rdur;
        g->slope = 4.0 * 1.0 * (rdur - rdur2);
        g->curve = -8.0 * 1.0 * rdur2;
        break;
    }
    case (LOOPER_ENV_TRAPEZOIDAL):
        g->amplitude_increment = 1.0 / g->attack_time_samples;
        break;
    case (LOOPER_ENV_EXPONENTIAL_CURVE):
        g->exp_mul =
            pow((g->exp_min + 1.0) / g->exp_min, 1.0 / g->attack_time_samples);
        g->exp_now = g->exp_min;
        break;
    case (LOOPER_ENV_LOGARITHMIC_CURVE):
        g->exp_mul =
            pow(g->exp_min / (g->exp_min + 1), 1.0 / g->attack_time_samples);
        g->exp_now = g->exp_min + 1;
        break;
    }

    g->amp = 0;
    double rdur = 1.0 / params.dur;
    double rdur2 = rdur * rdur;
    g->slope = 4.0 * 1.0 * (rdur - rdur2);
    g->curve = -8.0 * 1.0 * rdur2;

    double loop_len_ms = 1000. * params.dur / SAMPLE_RATE;
    double attack_time_ms = loop_len_ms / 100. * params.attack_pct;
    double release_time_ms = loop_len_ms / 100. * params.release_pct;
    // printf("ATTACKMS: %f RELEASEMS: %f\n", attack_time_ms,
    // release_time_ms);

    g->eg.Reset();
    g->eg.SetAttackTimeMsec(attack_time_ms);
    g->eg.SetDecayTimeMsec(0);
    g->eg.SetReleaseTimeMsec(release_time_ms);
    g->eg.StartEg();

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
    if (!g)
    {
        std::cerr << "NAE GRAIN YA NUMPTY!!\n";
        return out;
    }
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
        out.left = utils::LinTerp(0, 1, audio_buffer[read_idx],
                                  audio_buffer[read_next_idx], frac);
        out.left *= g->amp;
        out.right = out.left;
    }
    else if (num_channels == 2)
    {
        int read_next_idx = read_idx + 2;
        sound_grain_check_idx(&read_next_idx, audio_buffer_len);
        out.left = utils::LinTerp(0, 1, audio_buffer[read_idx],
                                  audio_buffer[read_next_idx], frac);
        out.left *= g->amp;

        int read_idx_right = read_idx + 1;
        sound_grain_check_idx(&read_idx_right, audio_buffer_len);
        int read_next_idx_right = read_idx_right + 2;
        sound_grain_check_idx(&read_next_idx_right, audio_buffer_len);
        out.right = utils::LinTerp(0, 1, audio_buffer[read_idx_right],
                                   audio_buffer[read_next_idx_right], frac);
        out.right *= g->amp;
    }

    g->audiobuffer_cur_pos += (g->incr * num_channels);
    double one_percent = g->grain_len_frames / 100.0;
    double percent_pos = g->grain_counter_frames / one_percent;

    switch (g->envelope_mode)
    {
    case (LOOPER_ENV_PARABOLIC):
        g->amp = g->amp + g->slope;
        g->slope = g->slope + g->curve;
        if (g->amp < 0)
            g->amp = 0;
        if (g->amp > 1.0)
            g->amp = 1.0;
        break;
    case (LOOPER_ENV_TRAPEZOIDAL):
        if (g->grain_counter_frames < g->attack_to_sustain_boundary_sample_idx)
            g->amp += g->amplitude_increment;
        else if (g->grain_counter_frames ==
                 g->attack_to_sustain_boundary_sample_idx)
            g->amp = 1.0;
        else if (g->grain_counter_frames ==
                 g->sustain_to_decay_boundary_sample_idx)
            g->amplitude_increment = -1. / g->release_time_samples;
        break;
    case (LOOPER_ENV_TUKEY_WINDOW):
        if (percent_pos < g->attack_time_pct)
        {
            g->amp = (1.0 + cos(M_PI + (M_PI *
                                        ((double)g->grain_counter_frames /
                                         g->attack_time_samples) *
                                        (1.0 / 2.0))));
        }
        else if (percent_pos > (100 - g->release_time_pct))
        {
            double attack_and_sustain_len_frames =
                g->grain_len_frames - g->release_time_samples;

            g->amp =
                cos(M_PI *
                    ((g->grain_counter_frames - attack_and_sustain_len_frames) /
                     g->release_time_samples) *
                    (1.0 / 2.0));
        }
        break;
    case (LOOPER_ENV_GENERATOR):
        g->amp = g->eg.DoEnvelope(NULL);
        if (percent_pos > (100 - g->release_time_pct))
            g->eg.NoteOff();
        break;
    case (LOOPER_ENV_EXPONENTIAL_CURVE):
        if (g->grain_counter_frames <
                g->attack_to_sustain_boundary_sample_idx ||
            g->grain_counter_frames > g->sustain_to_decay_boundary_sample_idx)
        {
            g->exp_now *= g->exp_mul;
            g->amp = (g->exp_now - g->exp_min);
        }
        else if (g->grain_counter_frames ==
                 g->attack_to_sustain_boundary_sample_idx)
        {
            g->amp = 1.;
        }
        else if (g->grain_counter_frames ==
                 g->sustain_to_decay_boundary_sample_idx)
        {
            g->exp_now = 1 + g->exp_min;
            g->exp_mul =
                pow(g->exp_min / (1 + g->exp_min), 1 / g->release_time_samples);
        }
        break;
    case (LOOPER_ENV_LOGARITHMIC_CURVE):
        if (g->grain_counter_frames <
                g->attack_to_sustain_boundary_sample_idx ||
            g->grain_counter_frames > g->sustain_to_decay_boundary_sample_idx)
        {
            g->exp_now *= g->exp_mul;
            g->amp = (1 - (g->exp_now - g->exp_min));
        }
        else if (g->grain_counter_frames ==
                 g->attack_to_sustain_boundary_sample_idx)
        {
            g->amp = 1.;
        }
        else if (g->grain_counter_frames ==
                 g->sustain_to_decay_boundary_sample_idx)
        {
            g->exp_now = g->exp_min;
            g->exp_mul =
                pow((g->exp_min + 1) / g->exp_min, 1 / g->release_time_samples);
        }
        break;
    }

    g->grain_counter_frames++;
    if (g->grain_counter_frames > g->grain_len_frames)
    {
        g->active = false;
    }

    return out;
}

// double sound_grain_env(sound_grain *g, unsigned int envelope_mode)
//{
//    if (!g->active)
//        return 0.;
//
//    double amp = 1;
//    double one_percent = g->grain_len_frames / 100.0;
//    double percent_pos = g->grain_counter_frames / one_percent;
//
//    switch (envelope_mode)
//    {
//    case (LOOPER_ENV_PARABOLIC):
//        g->amp = g->amp + g->slope;
//        g->slope = g->slope + g->curve;
//        amp = g->amp;
//        break;
//    case (LOOPER_ENV_TRAPEZOIDAL):
//        if (percent_pos < g->attack_time_pct)
//            amp *= (percent_pos / g->attack_time_pct);
//        else if (percent_pos > (100 - g->release_time_pct))
//            amp *= (100 - percent_pos) / g->release_time_pct;
//        break;
//    case (LOOPER_ENV_TUKEY_WINDOW):
//        if (percent_pos < g->attack_time_pct)
//        {
//            amp = (1.0 + cos(M_PI + (M_PI *
//                                     ((double)g->grain_counter_frames /
//                                      g->attack_time_samples) *
//                                     (1.0 / 2.0))));
//        }
//        else if (percent_pos > (100 - g->release_time_pct))
//        {
//            double attack_and_sustain_len_frames =
//                g->grain_len_frames - g->release_time_samples;
//
//            amp =
//                cos(M_PI *
//                    ((g->grain_counter_frames -
//                    attack_and_sustain_len_frames)
//                    /
//                     g->release_time_samples) *
//                    (1.0 / 2.0));
//        }
//        break;
//    case (LOOPER_ENV_GENERATOR):
//        amp = eg_do_envelope(&g->eg, NULL);
//        if (percent_pos > (100 - g->release_time_pct))
//            eg_note_off(&g->eg);
//        break;
//    }
//
//    if (g->debug)
//        printf("%f\n", amp);
//
//    return amp;
//}

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
void looper_set_incr_speed(looper *g, double speed) { g->incr_speed_ = speed; }

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
    g->loop_mode_ = m;
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

void looper_set_step_mode(looper *g, bool b)
{
    g->step_mode = b;
    if (b)
    {
        g->m_eg1.NoteOff();
    }
    else
    {
        g->m_eg1.StartEg();
    }
}

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
    m_eg1.StartEg();
}

void looper::noteOff(midi_event ev)
{
    (void)ev;
    m_eg1.NoteOff();
}

void looper::SetParam(std::string name, double val)
{
    if (name == "active")
    {
        this->active = val;
    }
    else if (name == "pitch")
        looper_set_grain_pitch(this, val);
    else if (name == "speed")
        looper_set_incr_speed(this, val);
    else if (name == "mode")
    {
        looper_set_loop_mode(this, val);
    }
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
