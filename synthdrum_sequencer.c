#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "mixer.h"
#include "sequencer_utils.h"
#include "synthdrum_sequencer.h"
#include "utils.h"

extern char *state_strings;
extern mixer *mixr;

// this must be a dupe - sure i already have this..
const char *s_synth_waves[] = {"SINE", "SAW1",   "SAW2",  "SAW3",
                               "TRI",  "SQUARE", "NOISE", "PNOISE"};
const char *s_sd_eg_state[] = {"OFF",     "ATTACK",  "DECAY",
                               "SUSTAIN", "RELEASE", "SHUTDOWN"};

synthdrum_sequencer *new_synthdrum_seq()
{
    printf("New Drum Synth!\n");
    synthdrum_sequencer *sds = calloc(1, sizeof(synthdrum_sequencer));
    step_init(&sds->m_seq);

    sds->vol = 0.6;
    sds->started = false;
    sds->reset_osc = true;

    strncpy(sds->m_patch_name, "Default", 7);

    osc_new_settings(&sds->m_osc1.osc);
    qb_set_soundgenerator_interface(&sds->m_osc1);
    sds->m_osc1.osc.m_waveform = NOISE;
    sds->m_osc1.osc.m_osc_fo = 58; // irrelevant when noise
    sds->osc1_amp = .01;
    osc_update(&sds->m_osc1.osc);

    // osc1 noise amp env
    envelope_generator_init(&sds->m_eg1);
    eg_set_attack_time_msec(&sds->m_eg1, 1);
    eg_set_decay_time_msec(&sds->m_eg1, 1);
    eg_set_release_time_msec(&sds->m_eg1, 10);
    eg_set_sustain_level(&sds->m_eg1, 0);

    osc_new_settings(&sds->m_osc2.osc);
    qb_set_soundgenerator_interface(&sds->m_osc2);
    sds->m_osc2.osc.m_waveform = SINE;
    sds->m_osc2.osc.m_osc_fo = 58;
    sds->osc2_amp = 1.0;
    osc_update(&sds->m_osc2.osc);

    // osc2 pitch envelope AND amp
    envelope_generator_init(&sds->m_eg2);
    eg_set_attack_time_msec(&sds->m_eg2, 1);
    eg_set_decay_time_msec(&sds->m_eg2, 50);
    eg_set_release_time_msec(&sds->m_eg2, 1);
    eg_set_sustain_level(&sds->m_eg2, 0);
    sds->eg2_osc2_intensity = 1;

    filter_moog_init(&sds->m_filter);
    sds->m_filter_type = LPF4;
    sds->m_filter_fc = 18000;
    sds->m_filter_q = 0.707;
    filter_set_type((filter *)&sds->m_filter, sds->m_filter_type);
    filter_set_fc_control((filter *)&sds->m_filter, sds->m_filter_fc);
    moog_set_qcontrol((filter *)&sds->m_filter, sds->m_filter_q);

    sds->m_distortion_threshold = 0.707;

    sds->sg.gennext = &sds_gennext;
    sds->sg.status = &sds_status;
    sds->sg.getvol = &sds_getvol;
    sds->sg.setvol = &sds_setvol;
    sds->sg.start = &sds_start;
    sds->sg.stop = &sds_stop;
    sds->sg.get_num_patterns = &sds_get_num_patterns;
    sds->sg.make_active_track = &sds_make_active_track;
    sds->sg.self_destruct = &synthdrum_del_self;
    sds->sg.event_notify = &sds_event_notify;
    sds->sg.set_pattern = &synthdrum_set_pattern;
    sds->sg.get_pattern = &synthdrum_get_pattern;
    sds->sg.is_valid_pattern = &synthdrum_is_valid_pattern;
    sds->sg.type = SYNTHDRUM_TYPE;
    sds->mod_semitones_range = 4;
    sds_start(sds);

    return sds;
}

bool synthdrum_is_valid_pattern(void *self, int pattern_num)
{
    synthdrum_sequencer *seq = (synthdrum_sequencer *)self;
    return step_is_valid_pattern_num(&seq->m_seq, pattern_num);
}

void synthdrum_randomize(synthdrum_sequencer *sds)
{

    sds->m_osc1.osc.m_waveform = rand() % MAX_OSC;
    sds->m_osc1.osc.m_osc_fo = rand() % 1400;
    sds->osc1_amp = ((float)rand()) / RAND_MAX;
    osc_update(&sds->m_osc1.osc);

    sds->m_osc2.osc.m_waveform = rand() % MAX_OSC;
    sds->m_osc2.osc.m_osc_fo = rand() % 1400;
    sds->osc2_amp = ((float)rand()) / RAND_MAX;
    osc_update(&sds->m_osc2.osc);

    eg_set_attack_time_msec(&sds->m_eg1, rand() % 100);
    eg_set_decay_time_msec(&sds->m_eg1, rand() % 100);
    eg_set_release_time_msec(&sds->m_eg1, rand() % 10);
    eg_set_sustain_level(&sds->m_eg1, ((float)rand()) / RAND_MAX);
    sds->eg1_sustain_ms = rand() % 2000;
    sds->eg1_osc1_intensity = ((float)rand()) / RAND_MAX;

    // osc2 pitch envelope
    eg_set_attack_time_msec(&sds->m_eg2, rand() % 100);
    eg_set_decay_time_msec(&sds->m_eg2, rand() % 100);
    eg_set_release_time_msec(&sds->m_eg2, rand() % 100);
    eg_set_sustain_level(&sds->m_eg2, ((float)rand()) / RAND_MAX);
    sds->eg2_sustain_ms = rand() % 2000;
    sds->eg2_osc2_intensity = ((float)rand()) / RAND_MAX;

    sds->m_filter_type = rand() % NUM_FILTER_TYPES;
    sds->m_filter_fc = rand() % 18000;
    sds->m_filter_q = ((float)rand()) / RAND_MAX;

    filter_set_type((filter *)&sds->m_filter, sds->m_filter_type);
    filter_set_fc_control((filter *)&sds->m_filter, sds->m_filter_fc);
    moog_set_qcontrol((filter *)&sds->m_filter, sds->m_filter_q);
    sds->m_distortion_threshold = ((float)rand()) / RAND_MAX;
}
void sds_status(void *self, wchar_t *ss)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer *)self;
    char *INSTRUMENT_RED = ANSI_COLOR_RESET;
    char *INSTRUMENT_DEEP_RED = ANSI_COLOR_RESET;
    if (sds->sg.active)
    {
        INSTRUMENT_RED = ANSI_COLOR_RED;
        INSTRUMENT_DEEP_RED = ANSI_COLOR_DEEP_RED;
    }

    // clang-format off
    swprintf(ss, MAX_STATIC_STRING_SZ,
             WANSI_COLOR_WHITE "%s " "%s" "vol:%.2f reset:%d distortion_threshold:%.2f\n"
             "o1_wav:" "%s""%s" "%s" "(%d) o1_fo:%.2f o1_amp:%.2f e2_o2_int:%.2f\n"
             "e1_att:%.2f e1_dec:%.2f e1_sus_lvl:%.2f e1_rel:%.2f\n"
             "o2_wav:" "%s" "%s" "%s" "(%d) o2_fo:%.2f o2_amp:%.2f mod_pitch_semitones:%d\n"
             "e2_att:%.2f e2_dec:%.2f e2_sus_lvl:%.2f e2_rel:%.2f\n"
             "%s"
             "filter_type:%d freq:%.2f q:%.2f // debug:%s",

             sds->m_patch_name,
             INSTRUMENT_RED,
             sds->vol,
             sds->reset_osc,
             sds->m_distortion_threshold,

             ANSI_COLOR_WHITE,
             s_synth_waves[sds->m_osc1.osc.m_waveform],
             INSTRUMENT_RED,

             sds->m_osc1.osc.m_waveform, sds->m_osc1.osc.m_osc_fo,
             sds->osc1_amp, sds->eg2_osc2_intensity,
             sds->m_eg1.m_attack_time_msec,
             sds->m_eg1.m_decay_time_msec, sds->m_eg1.m_sustain_level,
             sds->m_eg1.m_release_time_msec,

             ANSI_COLOR_WHITE,
             s_synth_waves[sds->m_osc2.osc.m_waveform],
             INSTRUMENT_DEEP_RED,

             sds->m_osc2.osc.m_waveform, sds->m_osc2.osc.m_fo, sds->osc2_amp,
             sds->mod_semitones_range, sds->m_eg2.m_attack_time_msec,
             sds->m_eg2.m_decay_time_msec, sds->m_eg2.m_sustain_level,
             sds->m_eg2.m_release_time_msec,

             INSTRUMENT_RED,

             sds->m_filter_type,
             sds->m_filter_fc, sds->m_filter_q,
             sds->debug ? "true" : "false");
    // clang-format on

    wchar_t step_status_string[MAX_STATIC_STRING_SZ];
    memset(step_status_string, 0, MAX_STATIC_STRING_SZ);
    step_status(&sds->m_seq, step_status_string);
    wcscat(ss, step_status_string);
    wcscat(ss, WANSI_COLOR_RESET);
}

void sds_setvol(void *self, double v)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer *)self;
    sds->vol = v;
    return;
}

void sds_event_notify(void *self, unsigned int event_type)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer *)self;

    if (!sds->sg.active)
        return;

    int idx;
    switch (event_type)
    {
    case (TIME_START_OF_LOOP_TICK):
        sds->started = true;
        break;
    case (TIME_MIDI_TICK):
        if (sds->started)
        {
            idx = mixr->timing_info.midi_tick % PPBAR;
            if (sds->m_seq.patterns[sds->m_seq.cur_pattern][idx].event_type)
                sds_trigger(sds);
        }
        break;
    case (TIME_SIXTEENTH_TICK):
        if (sds->started)
            step_tick(&sds->m_seq); // TODO rename to pattern generation
        break;
    }
}

stereo_val sds_gennext(void *self)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer *)self;
    stereo_val out = {0, 0};

    if (!sds->started)
        return out;

    // TRANSIENT /////////////////
    // this is for the initial 'Click'
    double osc1_amp_env = eg_do_envelope(&sds->m_eg1, NULL);
    if (sds->m_eg1.m_state == SUSTAIN)
        eg_note_off(&sds->m_eg1);

    osc_update(&sds->m_osc1.osc);
    double osc1_out =
        qb_do_oscillate(&sds->m_osc1.osc, NULL) * sds->osc1_amp * osc1_amp_env;

    // BODY ///////////////////////////
    /// EG2 env provides mod pitch of OSC2 _AND_ AMP OUT
    double pitch_env = 0; // biased output
    double amp_out_env = eg_do_envelope(&sds->m_eg2, &pitch_env);
    if (sds->m_eg2.m_state == SUSTAIN)
        eg_note_off(&sds->m_eg2);

    sds->m_osc2.osc.m_fo_mod =
        sds->eg2_osc2_intensity * sds->mod_semitones_range * pitch_env;

    osc_update(&sds->m_osc2.osc);
    double osc2_out = qb_do_oscillate(&sds->m_osc2.osc, NULL) * sds->osc2_amp;

    ////////////////

    double combined_osc = (osc1_out + osc2_out) * amp_out_env;

    sds->m_distortion.m_threshold = sds->m_distortion_threshold;
    combined_osc = distortion_process(&sds->m_distortion, combined_osc);

    sds->m_filter.f.m_filter_type = sds->m_filter_type;
    sds->m_filter.f.m_fc_control = sds->m_filter_fc;
    sds->m_filter.f.m_q_control = sds->m_filter_q;
    moog_update((filter *)&sds->m_filter);
    combined_osc = moog_gennext((filter *)&sds->m_filter, combined_osc);

    combined_osc = effector(&sds->sg, combined_osc);

    out.left = combined_osc * sds->vol;
    out.right = combined_osc * sds->vol;

    return out;
}

double sds_getvol(void *self)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer *)self;
    return sds->vol;
}

void sds_trigger(synthdrum_sequencer *sds)
{
    // printf("trigger!\n");
    if (sds->reset_osc)
        osc_reset(&sds->m_osc1.osc);
    sds->m_osc1.osc.m_note_on = true;
    eg_start_eg(&sds->m_eg1);

    if (sds->reset_osc)
        osc_reset(&sds->m_osc2.osc);
    sds->m_osc2.osc.m_note_on = true;
    eg_start_eg(&sds->m_eg2);
}

bool synthdrum_save_patch(synthdrum_sequencer *sds, char *name)
{
    if (strlen(name) == 0)
    {
        printf("Play tha game, pal, need a name to save yer synthdrum "
               "settings "
               "with\n");
        return false;
    }
    strncpy(sds->m_patch_name, name, 511);
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
            sds->m_patch_name, sds->vol, sds->m_distortion.m_threshold,

            sds->m_osc1.osc.m_waveform, sds->m_osc1.osc.m_osc_fo, sds->osc1_amp,
            sds->m_osc2.osc.m_waveform, sds->m_osc2.osc.m_osc_fo, sds->osc2_amp,

            sds->m_eg1.m_attack_time_msec, sds->m_eg1.m_decay_time_msec,
            sds->m_eg1.m_sustain_level, sds->m_eg1.m_release_time_msec,

            sds->m_eg2.m_attack_time_msec, sds->m_eg2.m_decay_time_msec,
            sds->m_eg2.m_sustain_level, sds->m_eg2.m_release_time_msec,
            sds->eg2_osc2_intensity,

            sds->m_filter_type, sds->m_filter_fc, sds->m_filter_q

    );

    fclose(filetosave);
    return true;
}

bool synthdrum_open_patch(synthdrum_sequencer *sds, char *name)
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
            printf("BEFORE OSC_FO %f\n", sds->m_osc1.osc.m_osc_fo);
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
                sds->m_patch_name, &sds->vol, &sds->m_distortion.m_threshold,

                &sds->m_osc1.osc.m_waveform, &sds->m_osc1.osc.m_osc_fo,
                &sds->osc1_amp, &sds->m_osc2.osc.m_waveform,
                &sds->m_osc2.osc.m_osc_fo, &sds->osc2_amp,

                &sds->m_eg1.m_attack_time_msec, &sds->m_eg1.m_decay_time_msec,
                &sds->m_eg1.m_sustain_level, &sds->m_eg1.m_release_time_msec,

                &sds->m_eg2.m_attack_time_msec, &sds->m_eg2.m_decay_time_msec,
                &sds->m_eg2.m_sustain_level, &sds->m_eg2.m_release_time_msec,
                &sds->eg2_osc2_intensity,

                &sds->m_filter_type, &sds->m_filter_fc, &sds->m_filter_q);

            printf("AFTER OSC_FO %f - scanned %d\n", sds->m_osc1.osc.m_osc_fo,
                   num);
        }
    }
    fclose(fp);
    return true;
}

bool synthdrum_list_patches()
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

void synthdrum_del_self(void *self)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer *)self;
    printf("Deleting Synthdrum self\n");
    free(sds);
}

void synthdrum_set_osc_wav(synthdrum_sequencer *sds, int osc_num,
                           unsigned int wave)
{
    if (!(wave < MAX_OSC))
    {
        printf("WAV has to be between 0 and %d\n", MAX_OSC - 1);
        return;
    }
    switch (osc_num)
    {
    case (1):
        sds->m_osc1.osc.m_waveform = wave;
        break;
    case (2):
        sds->m_osc2.osc.m_waveform = wave;
        break;
    }
}
void synthdrum_set_osc_fo(synthdrum_sequencer *sds, int osc_num, double freq)
{
    if (freq >= OSC_FO_MIN && freq <= OSC_FO_MAX)
    {
        switch (osc_num)
        {
        case (1):
            sds->m_osc1.osc.m_osc_fo = freq;
            break;
        case (2):
            sds->m_osc2.osc.m_osc_fo = freq;
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

void synthdrum_set_eg_attack(synthdrum_sequencer *sds, int eg_num, double val)
{
    if (val >= EG_MINTIME_MS && val <= EG_MAXTIME_MS)
    {
        switch (eg_num)
        {
        case (1):
            eg_set_attack_time_msec(&sds->m_eg1, val);
            break;
        case (2):
            eg_set_attack_time_msec(&sds->m_eg2, val);
            break;
        }
    }
    else
        printf("Val has to be between %d and %d\n", EG_MINTIME_MS,
               EG_MAXTIME_MS);
}
void synthdrum_set_eg_decay(synthdrum_sequencer *sds, int eg_num, double val)
{
    if (val >= EG_MINTIME_MS && val <= EG_MAXTIME_MS)
    {
        switch (eg_num)
        {
        case (1):
            eg_set_decay_time_msec(&sds->m_eg1, val);
            break;
        case (2):
            eg_set_decay_time_msec(&sds->m_eg2, val);
            break;
        }
    }
    else
        printf("Val has to be between %d and %d\n", EG_MINTIME_MS,
               EG_MAXTIME_MS);
}

void synthdrum_set_eg_sustain_lvl(synthdrum_sequencer *sds, int eg_num,
                                  double val)
{
    if (val >= 0. && val <= 1.)
    {
        switch (eg_num)
        {
        case (1):
            eg_set_sustain_level(&sds->m_eg1, val);
            break;
        case (2):
            eg_set_sustain_level(&sds->m_eg2, val);
            break;
        }
    }
    else
        printf("Val has to be between 0 and 1\n");
}

void synthdrum_set_eg_release(synthdrum_sequencer *sds, int eg_num, double val)
{
    if (val >= EG_MINTIME_MS && val <= EG_MAXTIME_MS)
    {
        switch (eg_num)
        {
        case (1):
            eg_set_release_time_msec(&sds->m_eg1, val);
            break;
        case (2):
            eg_set_release_time_msec(&sds->m_eg2, val);
            break;
        }
    }
    else
        printf("Val has to be between %d and %d\n", EG_MINTIME_MS,
               EG_MAXTIME_MS);
}

void synthdrum_set_eg_osc_intensity(synthdrum_sequencer *sds, int eg, int osc,
                                    double val)
{
    if (val >= -1 && val <= 1)
    {
        switch (eg)
        {
        case (1):
            sds->eg1_osc1_intensity = val;
            break;
        case (2):
            sds->eg2_osc2_intensity = val;
            break;
        }
    }
    else
        printf("Val has to be between -1 and 1\n");
}

void synthdrum_set_osc_amp(synthdrum_sequencer *sds, int osc_num, double val)
{
    if (val >= 0 && val <= 1.0)
    {
        switch (osc_num)
        {
        case (1):
            sds->osc1_amp = val;
            break;
        case (2):
            sds->osc2_amp = val;
            break;
        }
    }
    else
        printf("Val must be between 0 and 1\n");
}

void sds_start(void *self)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer *)self;
    sds->sg.active = true;
}

void sds_stop(void *self)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer *)self;
    sds->sg.active = false;
}

int sds_get_num_patterns(void *self)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer *)self;
    return sds->m_seq.num_patterns;
}

void sds_set_num_patterns(void *self, int num_patterns)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer *)self;
    sds->m_seq.num_patterns = num_patterns;
}

void sds_make_active_track(void *self, int track_num)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer *)self;
    sds->m_seq.cur_pattern = track_num;
}

void synthdrum_set_distortion_threshold(synthdrum_sequencer *sds, double val)
{
    if (val >= 0 && val <= 1)
        sds->m_distortion_threshold = val;
    else
        printf("Val must be between 0 and 1\n");
}

void synthdrum_set_filter_freq(synthdrum_sequencer *sds, double val)
{
    sds->m_filter_fc = val;
    filter_set_fc_control((filter *)&sds->m_filter, val);
}
void synthdrum_set_filter_q(synthdrum_sequencer *sds, double val)
{
    sds->m_filter_q = val;
    moog_set_qcontrol((filter *)&sds->m_filter, val);
}
void synthdrum_set_filter_type(synthdrum_sequencer *sds, unsigned int val)
{
    sds->m_filter_type = val;
    filter_set_type((filter *)&sds->m_filter, val);
}
void synthdrum_set_mod_semitones_range(synthdrum_sequencer *sds, int val)
{
    sds->mod_semitones_range = val;
}

midi_event *synthdrum_get_pattern(void *self, int pattern_num)
{
    synthdrum_sequencer *seq = (synthdrum_sequencer *)self;
    return step_get_pattern(&seq->m_seq, pattern_num);
}

void synthdrum_set_pattern(void *self, int pattern_num, midi_event *pattern)
{
    synthdrum_sequencer *seq = (synthdrum_sequencer *)self;
    return step_set_pattern(&seq->m_seq, pattern_num, pattern);
}

void synthdrum_set_reset_osc(synthdrum_sequencer *sds, bool b)
{
    sds->reset_osc = b;
}
void synthdrum_set_debug(synthdrum_sequencer *sds, bool debug)
{
    sds->debug = debug;
}
