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

drumsynth *new_drumsynth()
{
    printf("New Drum Synth!\n");
    drumsynth *ds = (drumsynth *) calloc(1, sizeof(drumsynth));

    ds->started = false;
    ds->reset_osc = true;

    strncpy(ds->m_patch_name, "Default", 7);

    osc_new_settings(&ds->m_osc1.osc);
    qb_set_sound_generator_interface(&ds->m_osc1);
    ds->m_osc1.osc.m_waveform = NOISE;
    ds->m_osc1.osc.m_osc_fo = 58; // irrelevant when noise
    ds->osc1_amp = .01;
    osc_update(&ds->m_osc1.osc);

    // osc1 noise amp env
    envelope_generator_init(&ds->m_eg1);
    eg_set_attack_time_msec(&ds->m_eg1, 1);
    eg_set_decay_time_msec(&ds->m_eg1, 1);
    eg_set_release_time_msec(&ds->m_eg1, 10);
    eg_set_sustain_level(&ds->m_eg1, 0);
    eg_set_drum_mode(&ds->m_eg1, true);

    osc_new_settings(&ds->m_osc2.osc);
    qb_set_sound_generator_interface(&ds->m_osc2);
    ds->m_osc2.osc.m_waveform = SINE;
    ds->m_osc2.osc.m_osc_fo = 58;
    ds->osc2_amp = 1.0;
    osc_update(&ds->m_osc2.osc);

    // osc2 pitch envelope AND amp
    envelope_generator_init(&ds->m_eg2);
    eg_set_attack_time_msec(&ds->m_eg2, 1);
    eg_set_decay_time_msec(&ds->m_eg2, 70);
    eg_set_release_time_msec(&ds->m_eg2, 1);
    eg_set_sustain_level(&ds->m_eg2, 0);
    eg_set_drum_mode(&ds->m_eg2, true);
    ds->eg2_osc2_intensity = 1;

    filter_moog_init(&ds->m_filter);
    ds->m_filter_type = LPF4;
    ds->m_filter_fc = 18000;
    ds->m_filter_q = 0.707;
    filter_set_type((filter *)&ds->m_filter, ds->m_filter_type);
    filter_set_fc_control((filter *)&ds->m_filter, ds->m_filter_fc);
    moog_set_qcontrol((filter *)&ds->m_filter, ds->m_filter_q);

    ds->m_distortion_threshold = 0.707;
    ds->mod_semitones_range = 12;

    ds->sg.gennext = &drumsynth_gennext;
    ds->sg.status = &drumsynth_status;
    ds->sg.get_volume = &sound_generator_get_volume;
    ds->sg.set_volume = &sound_generator_set_volume;
    ds->sg.get_pan = &sound_generator_get_pan;
    ds->sg.set_pan = &sound_generator_set_pan;
    ds->sg.start = &drumsynth_start;
    ds->sg.stop = &drumsynth_stop;
    ds->sg.self_destruct = &drumsynth_del_self;
    ds->sg.event_notify = &sequence_engine_event_notify;
    ds->sg.type = DRUMSYNTH_TYPE;

    sequence_engine_init(&ds->engine, (void *)ds, DRUMSYNTH_TYPE);
    sound_generator_init(&ds->sg);

    drumsynth_start(ds);

    // printf("DRUMSYNTH ACTIVE: %s\n", ds->sg.active ? "true" : "false");

    return ds;
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
void drumsynth_status(void *self, wchar_t *ss)
{
    drumsynth *ds = (drumsynth *)self;
    char *INSTRUMENT_RED = ANSI_COLOR_RESET;
    char *INSTRUMENT_DEEP_RED = ANSI_COLOR_RESET;
    if (ds->sg.active)
    {
        INSTRUMENT_RED = COOL_COLOR_YELLOW_MELLOW;
        INSTRUMENT_DEEP_RED = COOL_COLOR_ORANGE;
    }

    // clang-format off
    swprintf(ss, MAX_STATIC_STRING_SZ,
             WANSI_COLOR_WHITE "%s " "%s" "vol:%.2f pan:%.2f reset:%d distortion_threshold:%.2f\n"
             "o1_wav:" "%s""%s" "%s" "(%d) o1_fo:%.2f o1_amp:%.2f e2_o2_int:%.2f\n"
             "e1_att:%.2f e1_dec:%.2f e1_sus_lvl:%.2f e1_rel:%.2f\n"
             "o2_wav:" "%s" "%s" "%s" "(%d) o2_fo:%.2f o2_amp:%.2f mod_pitch_semitones:%d\n"
             "e2_att:%.2f e2_dec:%.2f e2_sus_lvl:%.2f e2_rel:%.2f\n"
             "%s"
             "filter_type:%d freq:%.2f q:%.2f // debug:%s",

             ds->m_patch_name,
             INSTRUMENT_RED,
             ds->sg.volume,
             ds->sg.pan,
             ds->reset_osc,
             ds->m_distortion_threshold,

             ANSI_COLOR_WHITE,
             s_synth_waves[ds->m_osc1.osc.m_waveform],
             INSTRUMENT_RED,

             ds->m_osc1.osc.m_waveform, ds->m_osc1.osc.m_osc_fo,
             ds->osc1_amp, ds->eg2_osc2_intensity,
             ds->m_eg1.m_attack_time_msec,
             ds->m_eg1.m_decay_time_msec, ds->m_eg1.m_sustain_level,
             ds->m_eg1.m_release_time_msec,

             ANSI_COLOR_WHITE,
             s_synth_waves[ds->m_osc2.osc.m_waveform],
             INSTRUMENT_DEEP_RED,

             ds->m_osc2.osc.m_waveform, ds->m_osc2.osc.m_fo, ds->osc2_amp,
             ds->mod_semitones_range, ds->m_eg2.m_attack_time_msec,
             ds->m_eg2.m_decay_time_msec, ds->m_eg2.m_sustain_level,
             ds->m_eg2.m_release_time_msec,

             INSTRUMENT_RED,

             ds->m_filter_type,
             ds->m_filter_fc, ds->m_filter_q,
             ds->debug ? "true" : "false");
    // clang-format on

    wchar_t engine_status_string[MAX_STATIC_STRING_SZ];
    memset(engine_status_string, 0, MAX_STATIC_STRING_SZ);
    sequence_engine_status(&ds->engine, engine_status_string);
    wcscat(ss, engine_status_string);
    wcscat(ss, WANSI_COLOR_RESET);
}

stereo_val drumsynth_gennext(void *self)
{
    drumsynth *ds = (drumsynth *)self;
    stereo_val out = {0, 0};

    // TRANSIENT /////////////////
    // this is for the initial 'Click'
    double osc1_amp_env = eg_do_envelope(&ds->m_eg1, NULL);

    osc_update(&ds->m_osc1.osc);
    double osc1_out =
        qb_do_oscillate(&ds->m_osc1.osc, NULL) * ds->osc1_amp * osc1_amp_env;

    // BODY ///////////////////////////
    /// EG2 env provides mod pitch of OSC2 _AND_ AMP OUT
    double pitch_env = 0; // biased output
    double amp_out_env = eg_do_envelope(&ds->m_eg2, &pitch_env);

    ds->m_osc2.osc.m_fo_mod =
        ds->eg2_osc2_intensity * ds->mod_semitones_range * pitch_env;

    osc_update(&ds->m_osc2.osc);
    double osc2_out = qb_do_oscillate(&ds->m_osc2.osc, NULL) * ds->osc2_amp;

    ////////////////

    double combined_osc = (osc1_out + osc2_out) * amp_out_env;

    ds->m_filter.f.m_filter_type = ds->m_filter_type;
    ds->m_filter.f.m_fc_control = ds->m_filter_fc;
    ds->m_filter.f.m_q_control = ds->m_filter_q;
    moog_update((filter *)&ds->m_filter);
    combined_osc = moog_gennext((filter *)&ds->m_filter, combined_osc);

    ds->sg.pan = fmin(ds->sg.pan, 1.0);
    ds->sg.pan = fmax(ds->sg.pan, -1.0);
    double pan_left = 0.707;
    double pan_right = 0.707;
    calculate_pan_values(ds->sg.pan, &pan_left, &pan_right);

    double midi_velocity = scaleybum(0, 127, 0, 1, ds->current_velocity);
    out.left = combined_osc * ds->sg.volume * midi_velocity * pan_left;
    out.right = combined_osc * ds->sg.volume * midi_velocity * pan_right;

    ds->m_distortion.m_threshold = ds->m_distortion_threshold;
    out = distortion_process(&ds->m_distortion, out);

    out = effector(&ds->sg, out);

    return out;
}

void drumsynth_trigger(drumsynth *ds)
{
    if (ds->reset_osc)
        osc_reset(&ds->m_osc1.osc);
    ds->m_osc1.osc.m_note_on = true;
    eg_start_eg(&ds->m_eg1);

    if (ds->reset_osc)
        osc_reset(&ds->m_osc2.osc);
    ds->m_osc2.osc.m_note_on = true;
    eg_start_eg(&ds->m_eg2);
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
            ds->m_patch_name, ds->sg.volume, ds->m_distortion_threshold,

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
                ds->m_patch_name, &ds->sg.volume, &ds->m_distortion_threshold,

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

void drumsynth_del_self(void *self)
{
    drumsynth *ds = (drumsynth *)self;
    printf("Deleting drumsynth self\n");
    free(ds);
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

void drumsynth_start(void *self)
{
    drumsynth *ds = (drumsynth *)self;
    ds->sg.active = true;
}

void drumsynth_stop(void *self)
{
    drumsynth *ds = (drumsynth *)self;
    ds->sg.active = false;
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
