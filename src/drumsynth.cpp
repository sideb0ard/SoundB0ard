#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "drumsynth.h"
#include "mixer.h"
#include "utils.h"

extern char *state_strings;
extern mixer *mixr;

// this must be a dupe - sure i already have this..
const char *s_synth_waves[] = {"SINE", "SAW1",   "SAW2",  "SAW3",
                               "TRI",  "SQUARE", "NOISE", "PNOISE"};
const char *s_ds_eg_state[] = {"OFF",     "ATTACK",  "DECAY",
                               "SUSTAIN", "RELEASE", "SHUTDOWN"};

drumsynth::drumsynth()
{
    printf("New Drum Synth!\n");
    started = false;
    reset_osc = true;

    strncpy(m_patch_name, "Default", 7);

    osc_new_settings(&m_osc1.osc);
    qb_set_sound_generator_interface(&m_osc1);
    m_osc1.osc.m_waveform = NOISE;
    m_osc1.osc.m_osc_fo = 58; // irrelevant when noise
    osc1_amp = .01;
    osc_update(&m_osc1.osc);

    // osc1 noise amp env
    envelope_generator_init(&m_eg1);
    eg_set_attack_time_msec(&m_eg1, 1);
    eg_set_decay_time_msec(&m_eg1, 1);
    eg_set_release_time_msec(&m_eg1, 10);
    eg_set_sustain_level(&m_eg1, 0);
    eg_set_drum_mode(&m_eg1, true);

    osc_new_settings(&m_osc2.osc);
    qb_set_sound_generator_interface(&m_osc2);
    m_osc2.osc.m_waveform = SINE;
    m_osc2.osc.m_osc_fo = 58;
    osc2_amp = 1.0;
    osc_update(&m_osc2.osc);

    // osc2 pitch envelope AND amp
    envelope_generator_init(&m_eg2);
    eg_set_attack_time_msec(&m_eg2, 1);
    eg_set_decay_time_msec(&m_eg2, 70);
    eg_set_release_time_msec(&m_eg2, 1);
    eg_set_sustain_level(&m_eg2, 0);
    eg_set_drum_mode(&m_eg2, true);
    eg2_osc2_intensity = 1;

    filter_moog_init(&m_filter);
    m_filter_type = LPF4;
    m_filter_fc = 18000;
    m_filter_q = 0.707;
    filter_set_type((filter *)&m_filter, m_filter_type);
    filter_set_fc_control((filter *)&m_filter, m_filter_fc);
    moog_set_qcontrol((filter *)&m_filter, m_filter_q);

    m_distortion_threshold = 0.707;
    mod_semitones_range = 12;

    type = DRUMSYNTH_TYPE;

    start();
}

stereo_val drumsynth::genNext()
{
    stereo_val out = {0, 0};

    // TRANSIENT /////////////////
    // this is for the initial 'Click'
    double osc1_amp_env = eg_do_envelope(&m_eg1, NULL);

    osc_update(&m_osc1.osc);
    double osc1_out =
        qb_do_oscillate(&m_osc1.osc, NULL) * osc1_amp * osc1_amp_env;

    // BODY ///////////////////////////
    /// EG2 env provides mod pitch of OSC2 _AND_ AMP OUT
    double pitch_env = 0; // biased output
    double amp_out_env = eg_do_envelope(&m_eg2, &pitch_env);

    m_osc2.osc.m_fo_mod = eg2_osc2_intensity * mod_semitones_range * pitch_env;

    osc_update(&m_osc2.osc);
    double osc2_out = qb_do_oscillate(&m_osc2.osc, NULL) * osc2_amp;

    ////////////////

    double combined_osc = (osc1_out + osc2_out) * amp_out_env;

    m_filter.f.m_filter_type = m_filter_type;
    m_filter.f.m_fc_control = m_filter_fc;
    m_filter.f.m_q_control = m_filter_q;
    moog_update((filter *)&m_filter);
    combined_osc = moog_gennext((filter *)&m_filter, combined_osc);

    pan = fmin(pan, 1.0);
    pan = fmax(pan, -1.0);
    double pan_left = 0.707;
    double pan_right = 0.707;
    calculate_pan_values(pan, &pan_left, &pan_right);

    double midi_velocity = scaleybum(0, 127, 0, 1, current_velocity);
    out.left = combined_osc * volume * midi_velocity * pan_left;
    out.right = combined_osc * volume * midi_velocity * pan_right;

    m_distortion.SetParam("threshold", m_distortion_threshold);
    out = m_distortion.Process(out);

    out = Effector(out);

    return out;
}

std::string drumsynth::Info()
{
    std::stringstream ss;

    // char *INSTRUMENT_RED = (char *)ANSI_COLOR_RESET;
    // char *INSTRUMENT_DEEP_RED = (char *)ANSI_COLOR_RESET;
    // if (active)
    //{
    //    INSTRUMENT_RED = (char *)COOL_COLOR_YELLOW_MELLOW;
    //    INSTRUMENT_DEEP_RED = (char *)COOL_COLOR_ORANGE;
    //}

    //// clang-format off
    // swprintf(ss, MAX_STATIC_STRING_SZ,
    //         WANSI_COLOR_WHITE "%s " "%s" "vol:%.2f pan:%.2f reset:%d
    //         distortion_threshold:%.2f\n" "o1_wav:" "%s""%s" "%s" "(%d)
    //         o1_fo:%.2f o1_amp:%.2f e2_o2_int:%.2f\n" "e1_att:%.2f e1_dec:%.2f
    //         e1_sus_lvl:%.2f e1_rel:%.2f\n" "o2_wav:" "%s" "%s" "%s" "(%d)
    //         o2_fo:%.2f o2_amp:%.2f mod_pitch_semitones:%d\n" "e2_att:%.2f
    //         e2_dec:%.2f e2_sus_lvl:%.2f e2_rel:%.2f\n"
    //         "%s"
    //         "filter_type:%d freq:%.2f q:%.2f // debug:%s",

    //         m_patch_name,
    //         INSTRUMENT_RED,
    //         volume,
    //         pan,
    //         reset_osc,
    //         m_distortion_threshold,

    //         ANSI_COLOR_WHITE,
    //         s_synth_waves[m_osc1.osc.m_waveform],
    //         INSTRUMENT_RED,

    //         m_osc1.osc.m_waveform, m_osc1.osc.m_osc_fo,
    //         osc1_amp, eg2_osc2_intensity,
    //         m_eg1.m_attack_time_msec,
    //         m_eg1.m_decay_time_msec, m_eg1.m_sustain_level,
    //         m_eg1.m_release_time_msec,

    //         ANSI_COLOR_WHITE,
    //         s_synth_waves[m_osc2.osc.m_waveform],
    //         INSTRUMENT_DEEP_RED,

    //         m_osc2.osc.m_waveform, m_osc2.osc.m_fo, osc2_amp,
    //         mod_semitones_range, m_eg2.m_attack_time_msec,
    //         m_eg2.m_decay_time_msec, m_eg2.m_sustain_level,
    //         m_eg2.m_release_time_msec,

    //         INSTRUMENT_RED,

    //         m_filter_type,
    //         m_filter_fc, m_filter_q,
    //         debug ? "true" : "false");
    //// clang-format on

    // wchar_t engine_status_string[MAX_STATIC_STRING_SZ];
    // memset(engine_status_string, 0, MAX_STATIC_STRING_SZ);
    // sequence_engine_status(&engine, engine_status_string);
    // wcscat(ss, engine_status_string);
    // wcscat(ss, WANSI_COLOR_RESET);
    return ss.str();
}

std::string drumsynth::Status()
{
    std::stringstream ss;
    ss << "TODO";
    return ss.str();
}

void drumsynth::noteOn(midi_event ev)
{
    (void)ev;
    if (reset_osc)
    {
        osc_reset(&m_osc1.osc);
        osc_reset(&m_osc2.osc);
    }

    m_osc1.osc.m_note_on = true;
    eg_start_eg(&m_eg1);

    m_osc2.osc.m_note_on = true;
    eg_start_eg(&m_eg2);
}

void drumsynth_randomize(drumsynth *ds)
{

    ds->m_osc1.osc.m_waveform = rand() % MAX_OSC;
    ds->m_osc1.osc.m_osc_fo = rand() % 1400;
    ds->osc1_amp = ((float)rand()) / RAND_MAX;
    osc_update(&ds->m_osc1.osc);

    ds->m_osc2.osc.m_waveform = rand() % MAX_OSC;
    ds->m_osc2.osc.m_osc_fo = rand() % 1400;
    ds->osc2_amp = ((float)rand()) / RAND_MAX;
    osc_update(&ds->m_osc2.osc);

    eg_set_attack_time_msec(&ds->m_eg1, rand() % 100);
    eg_set_decay_time_msec(&ds->m_eg1, rand() % 100);
    eg_set_release_time_msec(&ds->m_eg1, rand() % 10);
    eg_set_sustain_level(&ds->m_eg1, ((float)rand()) / RAND_MAX);
    ds->eg1_sustain_ms = rand() % 2000;
    ds->eg1_osc1_intensity = ((float)rand()) / RAND_MAX;

    // osc2 pitch envelope
    eg_set_attack_time_msec(&ds->m_eg2, rand() % 100);
    eg_set_decay_time_msec(&ds->m_eg2, rand() % 100);
    eg_set_release_time_msec(&ds->m_eg2, rand() % 100);
    eg_set_sustain_level(&ds->m_eg2, ((float)rand()) / RAND_MAX);
    ds->eg2_sustain_ms = rand() % 2000;
    ds->eg2_osc2_intensity = ((float)rand()) / RAND_MAX;

    ds->m_filter_type = rand() % NUM_FILTER_TYPES;
    ds->m_filter_fc = rand() % 18000;
    ds->m_filter_q = ((float)rand()) / RAND_MAX;

    filter_set_type((filter *)&ds->m_filter, ds->m_filter_type);
    filter_set_fc_control((filter *)&ds->m_filter, ds->m_filter_fc);
    moog_set_qcontrol((filter *)&ds->m_filter, ds->m_filter_q);
    ds->m_distortion_threshold = ((float)rand()) / RAND_MAX;
}

bool drumsynth_save_patch(drumsynth *ds, char *name)
{
    if (strlen(name) == 0)
    {
        printf("Play tha game, pal, need a name to save yer drumsynth "
               "settings "
               "with\n");
        return false;
    }
    strncpy(ds->m_patch_name, name, 511);
    printf("Saving '%s' settings for Drumsynth to file %s\n", name,
           DRUMSYNTH_SAVED_SETUPS_FILENAME);
    FILE *filetosave = fopen(DRUMSYNTH_SAVED_SETUPS_FILENAME, "a");

    fprintf(filetosave,
            "%s"     // m_patch_name
            " %f"    // vol
            " %f"    // distortion_threshold
            " %d"    // m_osc1.osc.m_waveform
            " %f"    // m_osc1.osc.m_osc_fo
            " %f"    // osc1_amp
            " %d"    // m_osc2.osc.m_waveform
            " %f"    // m_osc2.osc.m_osc_fo
            " %f"    // osc2_amp
            " %d"    // mod_semitones_range
            " %f"    // m_eg1.m_attack_time_msec
            " %f"    // m_eg1.m_decay_time_msec
            " %f"    // m_eg1.m_sustain_level
            " %f"    // m_eg1.m_release_time_msec
            " %f"    // m_eg2.m_attack_time_msec
            " %f"    // m_eg2.m_decay_time_msec
            " %f"    // m_eg2.m_sustain_level
            " %f"    // m_eg2.m_release_time_msec
            " %f"    // eg2_osc2_intensity
            " %d"    // filter type
            " %f"    // filter fc_control
            " %f\n", // filter q_control
            ds->m_patch_name, ds->volume, ds->m_distortion_threshold,

            ds->m_osc1.osc.m_waveform, ds->m_osc1.osc.m_osc_fo, ds->osc1_amp,
            ds->m_osc2.osc.m_waveform, ds->m_osc2.osc.m_osc_fo, ds->osc2_amp,

            ds->mod_semitones_range,

            ds->m_eg1.m_attack_time_msec, ds->m_eg1.m_decay_time_msec,
            ds->m_eg1.m_sustain_level, ds->m_eg1.m_release_time_msec,

            ds->m_eg2.m_attack_time_msec, ds->m_eg2.m_decay_time_msec,
            ds->m_eg2.m_sustain_level, ds->m_eg2.m_release_time_msec,
            ds->eg2_osc2_intensity,

            ds->m_filter_type, ds->m_filter_fc, ds->m_filter_q

    );

    fclose(filetosave);
    return true;
}

bool drumsynth_open_patch(drumsynth *ds, char *name)
{
    FILE *fp = fopen(DRUMSYNTH_SAVED_SETUPS_FILENAME, "r");
    if (fp == NULL)
    {
        printf("Dingie!\n");
        return false;
    }
    char line[256];
    char patch_name[52];
    while (fgets(line, sizeof(line), fp))
    {
        printf("%s", line);
        sscanf(line, "%s", patch_name);
        if (strncmp(patch_name, name, 255) == 0)
        {
            printf("MATCH PATCH NAME %s\n", patch_name);
            printf("BEFORE OSC_FO %f\n", ds->m_osc1.osc.m_osc_fo);
            int num = sscanf(
                line,
                "%s"      // m_patch_name
                " %lf"    // vol
                " %lf"    // distortion_threshold
                " %d"     // m_osc1.osc.m_waveform
                " %lf"    // m_osc1.osc.m_osc_fo
                " %lf"    // osc1_amp
                " %d"     // m_osc2.osc.m_waveform
                " %lf"    // m_osc2.osc.m_osc_fo
                " %lf"    // osc2_amp
                " %d"     // mod_semitones_range
                " %lf"    // m_eg1.m_attack_time_msec
                " %lf"    // m_eg1.m_decay_time_msec
                " %lf"    // m_eg1.m_sustain_level
                " %lf"    // m_eg1.m_release_time_msec
                " %lf"    // m_eg2.m_attack_time_msec
                " %lf"    // m_eg2.m_decay_time_msec
                " %lf"    // m_eg2.m_sustain_level
                " %lf"    // m_eg2.m_release_time_msec
                " %lf"    // eg2_osc2_intensity
                " %d"     // filter type
                " %lf"    // filter fc_control
                " %lf\n", // filter q_control
                ds->m_patch_name, &ds->volume, &ds->m_distortion_threshold,

                &ds->m_osc1.osc.m_waveform, &ds->m_osc1.osc.m_osc_fo,
                &ds->osc1_amp, &ds->m_osc2.osc.m_waveform,
                &ds->m_osc2.osc.m_osc_fo, &ds->osc2_amp,

                &ds->mod_semitones_range,

                &ds->m_eg1.m_attack_time_msec, &ds->m_eg1.m_decay_time_msec,
                &ds->m_eg1.m_sustain_level, &ds->m_eg1.m_release_time_msec,

                &ds->m_eg2.m_attack_time_msec, &ds->m_eg2.m_decay_time_msec,
                &ds->m_eg2.m_sustain_level, &ds->m_eg2.m_release_time_msec,
                &ds->eg2_osc2_intensity,

                &ds->m_filter_type, &ds->m_filter_fc, &ds->m_filter_q);

            printf("AFTER OSC_FO %f - scanned %d\n", ds->m_osc1.osc.m_osc_fo,
                   num);
        }
    }
    fclose(fp);
    return true;
}

bool drumsynth_list_patches()
{
    FILE *fp = fopen(DRUMSYNTH_SAVED_SETUPS_FILENAME, "r");
    if (fp == NULL)
    {
        printf("Dingie!\n");
        return false;
    }
    char line[256];
    char preset[256];

    char *tok, *last_tok;
    char const *sep = " ";

    while (fgets(line, sizeof(line), fp))
    {
        for (tok = strtok_r(line, sep, &last_tok); tok;
             tok = strtok_r(NULL, sep, &last_tok))
        {
            sscanf(tok, "%s", preset);
            printf("%s\n", preset);
            break;
        }
    }

    fclose(fp);
    return true;
}

void drumsynth_set_osc_wav(drumsynth *ds, int osc_num, unsigned int wave)
{
    if (!(wave < MAX_OSC))
    {
        printf("WAV has to be between 0 and %d\n", MAX_OSC - 1);
        return;
    }
    switch (osc_num)
    {
    case (1):
        ds->m_osc1.osc.m_waveform = wave;
        break;
    case (2):
        ds->m_osc2.osc.m_waveform = wave;
        break;
    }
}
void drumsynth_set_osc_fo(drumsynth *ds, int osc_num, double freq)
{
    if (freq >= OSC_FO_MIN && freq <= OSC_FO_MAX)
    {
        switch (osc_num)
        {
        case (1):
            ds->m_osc1.osc.m_osc_fo = freq;
            break;
        case (2):
            ds->m_osc2.osc.m_osc_fo = freq;
            break;
        }
    }
    else
    {
        printf("FREQ has to be between %d and %d\n", OSC_FO_MIN,
               OSC_FO_MAX - 1);
        return;
    }
}

void drumsynth_set_eg_attack(drumsynth *ds, int eg_num, double val)
{
    if (val >= EG_MINTIME_MS && val <= EG_MAXTIME_MS)
    {
        switch (eg_num)
        {
        case (1):
            eg_set_attack_time_msec(&ds->m_eg1, val);
            break;
        case (2):
            eg_set_attack_time_msec(&ds->m_eg2, val);
            break;
        }
    }
    else
        printf("Val has to be between %d and %d\n", EG_MINTIME_MS,
               EG_MAXTIME_MS);
}
void drumsynth_set_eg_decay(drumsynth *ds, int eg_num, double val)
{
    if (val >= EG_MINTIME_MS && val <= EG_MAXTIME_MS)
    {
        switch (eg_num)
        {
        case (1):
            eg_set_decay_time_msec(&ds->m_eg1, val);
            break;
        case (2):
            eg_set_decay_time_msec(&ds->m_eg2, val);
            break;
        }
    }
    else
        printf("Val has to be between %d and %d\n", EG_MINTIME_MS,
               EG_MAXTIME_MS);
}

void drumsynth_set_eg_sustain_lvl(drumsynth *ds, int eg_num, double val)
{
    if (val >= 0. && val <= 1.)
    {
        switch (eg_num)
        {
        case (1):
            eg_set_sustain_level(&ds->m_eg1, val);
            break;
        case (2):
            eg_set_sustain_level(&ds->m_eg2, val);
            break;
        }
    }
    else
        printf("Val has to be between 0 and 1\n");
}

void drumsynth_set_eg_release(drumsynth *ds, int eg_num, double val)
{
    if (val >= EG_MINTIME_MS && val <= EG_MAXTIME_MS)
    {
        switch (eg_num)
        {
        case (1):
            eg_set_release_time_msec(&ds->m_eg1, val);
            break;
        case (2):
            eg_set_release_time_msec(&ds->m_eg2, val);
            break;
        }
    }
    else
        printf("Val has to be between %d and %d\n", EG_MINTIME_MS,
               EG_MAXTIME_MS);
}

void drumsynth_set_eg_osc_intensity(drumsynth *ds, int eg, int osc, double val)
{
    (void)osc;
    if (val >= -1 && val <= 1)
    {
        switch (eg)
        {
        case (1):
            ds->eg1_osc1_intensity = val;
            break;
        case (2):
            ds->eg2_osc2_intensity = val;
            break;
        }
    }
    else
        printf("Val has to be between -1 and 1\n");
}

void drumsynth_set_osc_amp(drumsynth *ds, int osc_num, double val)
{
    if (val >= 0 && val <= 1.0)
    {
        switch (osc_num)
        {
        case (1):
            ds->osc1_amp = val;
            break;
        case (2):
            ds->osc2_amp = val;
            break;
        }
    }
    else
        printf("Val must be between 0 and 1\n");
}

void drumsynth_set_distortion_threshold(drumsynth *ds, double val)
{
    if (val >= 0 && val <= 1)
        ds->m_distortion_threshold = val;
    else
        printf("Val must be between 0 and 1\n");
}

void drumsynth_set_filter_freq(drumsynth *ds, double val)
{
    ds->m_filter_fc = val;
    filter_set_fc_control((filter *)&ds->m_filter, val);
}
void drumsynth_set_filter_q(drumsynth *ds, double val)
{
    ds->m_filter_q = val;
    moog_set_qcontrol((filter *)&ds->m_filter, val);
}
void drumsynth_set_filter_type(drumsynth *ds, unsigned int val)
{
    ds->m_filter_type = val;
    filter_set_type((filter *)&ds->m_filter, val);
}
void drumsynth_set_mod_semitones_range(drumsynth *ds, int val)
{
    ds->mod_semitones_range = val;
}

void drumsynth_set_reset_osc(drumsynth *ds, bool b) { ds->reset_osc = b; }
void drumsynth_set_debug(drumsynth *ds, bool debug) { ds->debug = debug; }

void drumsynth::SetParam(std::string name, double val) {}
double drumsynth::GetParam(std::string name) { return 0; }
