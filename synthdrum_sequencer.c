#include "synthdrum_sequencer.h"
#include "sequencer_utils.h"
#include "utils.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

extern char *state_strings;

synthdrum_sequencer *new_synthdrum_seq()
{
    printf("New Drum Synth!\n");
    synthdrum_sequencer *sds = calloc(1, sizeof(synthdrum_sequencer));
    seq_init(&sds->m_seq);

    sds->vol = 0.6;
    sds->started = false;
    for (int i = 0; i < SEQUENCER_PATTERN_LEN; i++)
    {
        sds->metadata[i].played = 0.0;
        sds->metadata[i].playing = 0.0;
    }

    strncpy(sds->m_patch_name, "Default", 7);

    // OSC1 transient noise
    osc_new_settings(&sds->m_osc1.osc);
    qb_set_soundgenerator_interface(&sds->m_osc1);
    sds->m_osc1.osc.m_waveform = NOISE;
    sds->osc1_amp = 0.307;
    osc_update(&sds->m_osc1.osc);

    // OSC body
    osc_new_settings(&sds->m_osc2.osc);
    qb_set_soundgenerator_interface(&sds->m_osc2);
    sds->m_osc2.osc.m_waveform = SINE;
    sds->m_osc2.osc.m_osc_fo = 94;
    sds->osc2_amp = 1.0;
    osc_update(&sds->m_osc2.osc);

    // osc1 noise amp env
    envelope_generator_init(&sds->m_eg1);
    eg_set_attack_time_msec(&sds->m_eg1, 1);
    eg_set_decay_time_msec(&sds->m_eg1, 0);
    eg_set_release_time_msec(&sds->m_eg1, 1);
    eg_set_sustain_level(&sds->m_eg1, 0.1);
    sds->eg1_sustain_ms = 0;
    sds->eg1_sustain_len_in_samples = SAMPLE_RATE / 1000. * sds->eg1_sustain_ms;
    sds->eg1_sustain_counter = 0;

    // osc2 pitch envelope
    envelope_generator_init(&sds->m_eg2);
    eg_set_attack_time_msec(&sds->m_eg2, 1);
    eg_set_decay_time_msec(&sds->m_eg2, 60);
    eg_set_release_time_msec(&sds->m_eg1, 100);
    eg_set_sustain_level(&sds->m_eg2, 0.1);
    sds->eg2_sustain_ms = 0;
    sds->eg2_sustain_len_in_samples = SAMPLE_RATE / 1000. * sds->eg2_sustain_ms;
    sds->eg2_sustain_counter = 0;
    sds->eg2_osc2_intensity = 1;

    // output amp envelope
    envelope_generator_init(&sds->m_eg3);
    eg_set_attack_time_msec(&sds->m_eg3, 1);
    eg_set_decay_time_msec(&sds->m_eg3, 250);
    eg_set_release_time_msec(&sds->m_eg3, 100);
    eg_set_sustain_level(&sds->m_eg3, 0.1);
    sds->eg3_sustain_ms = 0;
    sds->eg3_sustain_len_in_samples = SAMPLE_RATE / 1000. * sds->eg3_sustain_ms;
    sds->eg3_sustain_counter = 0;

    filter_moog_init(&sds->m_filter);
    sds->m_filter_type = LPF4;
    sds->m_filter_fc = 180;
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
    sds->sg.get_num_tracks = &sds_get_num_tracks;
    sds->sg.make_active_track = &sds_make_active_track;
    sds->sg.self_destruct = &synthdrum_del_self;
    sds->sg.type = SYNTHDRUM_TYPE;
    sds->mod_semitones_range = 4;
    sds_start(sds);

    return sds;
}

void synthdrum_randomize(synthdrum_sequencer *sds)
{

    sds->m_osc1.osc.m_waveform = rand() % MAX_OSC;
    sds->m_osc1.osc.m_osc_fo = rand() % 140;
    sds->osc1_amp = ((float)rand()) / RAND_MAX;
    osc_update(&sds->m_osc1.osc);

    sds->m_osc2.osc.m_waveform = rand() % MAX_OSC;
    sds->m_osc2.osc.m_osc_fo = rand() % 140;
    sds->osc2_amp = ((float)rand()) / RAND_MAX;
    osc_update(&sds->m_osc2.osc);

    eg_set_attack_time_msec(&sds->m_eg1, rand() % 100);
    eg_set_decay_time_msec(&sds->m_eg1, rand() % 100);
    eg_set_release_time_msec(&sds->m_eg1, rand() % 10);
    eg_set_sustain_level(&sds->m_eg1, ((float)rand()) / RAND_MAX);
    sds->eg1_sustain_ms = rand() % 200;
    sds->eg1_sustain_len_in_samples = SAMPLE_RATE / 1000. * sds->eg1_sustain_ms;
    sds->eg1_sustain_counter = 0;
    sds->eg2_osc2_intensity = ((float)rand()) / RAND_MAX;

    // osc2 pitch envelope
    eg_set_attack_time_msec(&sds->m_eg2, rand() % 100);
    eg_set_decay_time_msec(&sds->m_eg2, rand() % 100);
    eg_set_release_time_msec(&sds->m_eg2, rand() % 100);
    eg_set_sustain_level(&sds->m_eg2, ((float)rand()) / RAND_MAX);
    sds->eg2_sustain_ms = rand() % 200;
    sds->eg2_sustain_len_in_samples = SAMPLE_RATE / 1000. * sds->eg2_sustain_ms;
    sds->eg2_sustain_counter = 0;
    sds->eg2_osc2_intensity = ((float)rand()) / RAND_MAX;

    // output amp envelope
    eg_set_attack_time_msec(&sds->m_eg3, rand() % 100);
    eg_set_decay_time_msec(&sds->m_eg3, rand() % 100);
    eg_set_release_time_msec(&sds->m_eg3, rand() % 100);
    eg_set_sustain_level(&sds->m_eg3, ((float)rand()) / RAND_MAX);
    sds->eg3_sustain_ms = rand() % 200;
    sds->eg3_sustain_len_in_samples = SAMPLE_RATE / 1000. * sds->eg3_sustain_ms;
    sds->eg3_sustain_counter = 0;

    sds->m_filter_type = rand() % NUM_FILTER_TYPES;
    sds->m_filter_fc = rand() % 1800;
    sds->m_filter_q = ((float)rand()) / RAND_MAX;

    filter_set_type((filter *)&sds->m_filter, sds->m_filter_type);
    filter_set_fc_control((filter *)&sds->m_filter, sds->m_filter_fc);
    moog_set_qcontrol((filter *)&sds->m_filter, sds->m_filter_q);
    sds->m_distortion_threshold = ((float)rand()) / RAND_MAX;
}
void sds_status(void *self, wchar_t *ss)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer *)self;
    swprintf(ss, MAX_PS_STRING_SZ,
             L"[SYNTHDRUM] Name: %s Vol: %.2f distortion_threshold:%.2f "
             "\n      Osc1 osc1_wav:%d osc1_fo:%.2f osc1_amp:%.2f"
             "\n      eg1_attack:%.2f eg1_decay:%.2f eg1_sustain_level:%.2f "
             "eg1_sustain_ms:%.2f eg1_release:%.2f "
             "\n      Osc2 osc2_wav:%d osc2_fo:%.2f osc2_amp:%.2f "
             "mod_pitch_semitones:%d"
             "\n      eg2_attack:%.2f eg2_decay:%.2f eg2_sustain_level:%.2f "
             "eg2_sustain_ms:%.2f eg2_release:%.2f "
             "eg2_osc2_int:%.2f"
             "\n      eg3_attack:%.2f eg3_decay:%.2f eg3_sustain_level:%.2f "
             "eg3_sustain_ms:%.2f eg3_release:%.2f"
             "\n      filter_type:%d freq:%2.f q:%2.f",

             sds->m_patch_name, sds->vol, sds->m_distortion_threshold,

             sds->m_osc1.osc.m_waveform, sds->m_osc1.osc.m_osc_fo,
             sds->osc1_amp, sds->m_eg1.m_attack_time_msec,
             sds->m_eg1.m_decay_time_msec, sds->m_eg1.m_sustain_level,
             sds->eg1_sustain_len_in_samples / (SAMPLE_RATE / 1000.),
             sds->m_eg1.m_release_time_msec,

             sds->m_osc2.osc.m_waveform, sds->m_osc2.osc.m_fo, sds->osc2_amp,
             sds->mod_semitones_range, sds->m_eg2.m_attack_time_msec,
             sds->m_eg2.m_decay_time_msec, sds->m_eg2.m_sustain_level,
             sds->eg2_sustain_len_in_samples / (SAMPLE_RATE / 1000.),
             sds->m_eg2.m_release_time_msec, sds->eg2_osc2_intensity,

             sds->m_eg3.m_attack_time_msec, sds->m_eg3.m_decay_time_msec,
             sds->m_eg3.m_sustain_level,
             sds->eg3_sustain_len_in_samples / (SAMPLE_RATE / 1000.),
             sds->m_eg3.m_release_time_msec, sds->m_filter_type,
             sds->m_filter_fc, sds->m_filter_q);

    wchar_t seq_status_string[MAX_PS_STRING_SZ];
    memset(seq_status_string, 0, MAX_PS_STRING_SZ);
    seq_status(&sds->m_seq, seq_status_string);
    wcscat(ss, seq_status_string);
    wcscat(ss, WANSI_COLOR_RESET);
}

void sds_setvol(void *self, double v)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer *)self;
    sds->vol = v;
    return;
}

stereo_val sds_gennext(void *self, mixer_timing_info timing_info)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer *)self;
    stereo_val out = {0, 0};

    if (!sds->sg.active)
    {
        return out;
    }

    int idx = timing_info.midi_tick % PPBAR;

    if (!sds->started)
    {
        if (idx == 0)
            sds->started = true;
        else
            return out;
    }

    if (timing_info.is_midi_tick)
    {
        if (sds->m_seq.patterns[sds->m_seq.cur_pattern][idx])
            sds_trigger(sds);
    }
    seq_tick(&sds->m_seq, timing_info);

    // END POSITIONAL /////////////////////////////////////////

    if (sds->m_eg1.m_state == SUSTAIN)
    {
        sds->eg1_sustain_counter++;
        if (sds->eg1_sustain_counter >= sds->eg1_sustain_len_in_samples)
        {
            sds->eg1_sustain_counter = 0;
            sds->m_eg1.m_state = RELEASE;
        }
    }
    if (sds->m_eg2.m_state == SUSTAIN)
    {
        sds->eg2_sustain_counter++;
        if (sds->eg2_sustain_counter >= sds->eg2_sustain_len_in_samples)
        {
            sds->eg2_sustain_counter = 0;
            sds->m_eg2.m_state = RELEASE;
        }
    }
    if (sds->m_eg3.m_state == SUSTAIN)
    {
        sds->eg3_sustain_counter++;
        if (sds->eg3_sustain_counter >= sds->eg3_sustain_len_in_samples)
        {
            sds->eg3_sustain_counter = 0;
            sds->m_eg3.m_state = RELEASE;
        }
    }

    // noise env for initial snap
    double eg1_out = eg_do_envelope(&sds->m_eg1, NULL);
    osc_update(&sds->m_osc1.osc);
    double osc1_out =
        qb_do_oscillate(&sds->m_osc1.osc, NULL) * eg1_out * sds->osc1_amp;

    // eg2 env -> pitch of OSC2
    double eg2_biased_out = 0;
    eg_do_envelope(&sds->m_eg2, &eg2_biased_out);
    double eg2_osc_mod =
        sds->eg2_osc2_intensity * sds->mod_semitones_range * eg2_biased_out;
    sds->m_osc2.osc.m_fo_mod = eg2_osc_mod;

    osc_update(&sds->m_osc2.osc);
    double osc2_out = qb_do_oscillate(&sds->m_osc2.osc, NULL) * sds->osc2_amp;

    sds->m_distortion.m_threshold = sds->m_distortion_threshold;
    double distorted_out = distortion_process(&sds->m_distortion, osc2_out);

    // sds->m_filter.f.m_filter_type = sds->m_filter_type;
    // sds->m_filter.f.m_fc_control = sds->m_filter_fc;
    // sds->m_filter.f.m_q_control = sds->m_filter_q;
    moog_update((filter *)&sds->m_filter);
    double filtered_osc2_out =
        moog_gennext((filter *)&sds->m_filter, distorted_out);

    // overall amp env
    double amp_out_env = eg_do_envelope(&sds->m_eg3, NULL);

    double almost_out =
        (osc1_out * sds->osc1_amp + filtered_osc2_out * sds->osc2_amp) *
        amp_out_env * sds->vol;

    almost_out = effector(&sds->sg, almost_out);
    almost_out = envelopor(&sds->sg, almost_out);

    out.left = almost_out;
    out.right = almost_out;

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
    osc_reset(&sds->m_osc1.osc);
    sds->m_osc1.osc.m_note_on = true;
    eg_start_eg(&sds->m_eg1);
    sds->eg1_sustain_counter = 0;

    osc_reset(&sds->m_osc2.osc);
    sds->m_osc2.osc.m_note_on = true;
    eg_start_eg(&sds->m_eg2);
    sds->eg2_sustain_counter = 0;

    eg_start_eg(&sds->m_eg3);
    sds->eg3_sustain_counter = 0;
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

    fprintf(filetosave, "%s"     // m_patch_name
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
                        " %f"    // eg1_sustain_len_in_samples
                        " %f"    // m_eg1.m_release_time_msec
                        " %f"    // m_eg2.m_attack_time_msec
                        " %f"    // m_eg2.m_decay_time_msec
                        " %f"    // m_eg2.m_sustain_level
                        " %f"    // eg2_sustain_len_in_samples
                        " %f"    // m_eg2.m_release_time_msec
                        " %f"    // eg2_osc2_intensity
                        " %f"    // m_eg3.m_attack_time_msec
                        " %f"    // m_eg3.m_decay_time_msec
                        " %f"    // m_eg2.m_sustain_level
                        " %f"    // eg3_sustain_len_in_samples
                        " %f"    // m_eg3.m_release_time_msec
                        " %d"    // filter type
                        " %f"    // filter fc_control
                        " %f\n", // filter q_control
            sds->m_patch_name, sds->vol, sds->m_distortion.m_threshold,

            sds->m_osc1.osc.m_waveform, sds->m_osc1.osc.m_osc_fo, sds->osc1_amp,
            sds->m_osc2.osc.m_waveform, sds->m_osc2.osc.m_osc_fo, sds->osc2_amp,

            sds->m_eg1.m_attack_time_msec, sds->m_eg1.m_decay_time_msec,
            sds->m_eg1.m_sustain_level, sds->eg1_sustain_len_in_samples,
            sds->m_eg1.m_release_time_msec,

            sds->m_eg2.m_attack_time_msec, sds->m_eg2.m_decay_time_msec,
            sds->m_eg2.m_sustain_level, sds->eg2_sustain_len_in_samples,
            sds->m_eg2.m_release_time_msec, sds->eg2_osc2_intensity,

            sds->m_eg3.m_attack_time_msec, sds->m_eg3.m_decay_time_msec,
            sds->m_eg3.m_sustain_level, sds->eg3_sustain_len_in_samples,
            sds->m_eg3.m_release_time_msec,

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
                line, "%s"      // m_patch_name
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
                      " %lf"    // eg1_sustain_len_in_samples
                      " %lf"    // m_eg1.m_release_time_msec
                      " %lf"    // m_eg2.m_attack_time_msec
                      " %lf"    // m_eg2.m_decay_time_msec
                      " %lf"    // m_eg2.m_sustain_level
                      " %lf"    // eg2_sustain_len_in_samples
                      " %lf"    // m_eg2.m_release_time_msec
                      " %lf"    // eg2_osc2_intensity
                      " %lf"    // m_eg3.m_attack_time_msec
                      " %lf"    // m_eg3.m_decay_time_msec
                      " %lf"    // m_eg2.m_sustain_level
                      " %lf"    // eg3_sustain_len_in_samples
                      " %lf"    // m_eg3.m_release_time_msec
                      " %d"     // filter type
                      " %lf"    // filter fc_control
                      " %lf\n", // filter q_control
                sds->m_patch_name, &sds->vol, &sds->m_distortion.m_threshold,

                &sds->m_osc1.osc.m_waveform, &sds->m_osc1.osc.m_osc_fo,
                &sds->osc1_amp, &sds->m_osc2.osc.m_waveform,
                &sds->m_osc2.osc.m_osc_fo, &sds->osc2_amp,

                &sds->m_eg1.m_attack_time_msec, &sds->m_eg1.m_decay_time_msec,
                &sds->m_eg1.m_sustain_level, &sds->eg1_sustain_len_in_samples,
                &sds->m_eg1.m_release_time_msec,

                &sds->m_eg2.m_attack_time_msec, &sds->m_eg2.m_decay_time_msec,
                &sds->m_eg2.m_sustain_level, &sds->eg2_sustain_len_in_samples,
                &sds->m_eg2.m_release_time_msec, &sds->eg2_osc2_intensity,

                &sds->m_eg3.m_attack_time_msec, &sds->m_eg3.m_decay_time_msec,
                &sds->m_eg3.m_sustain_level, &sds->eg3_sustain_len_in_samples,
                &sds->m_eg3.m_release_time_msec,

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
    while (fgets(line, sizeof(line), fp))
    {
        printf("%s", line);
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
        case (3):
            eg_set_attack_time_msec(&sds->m_eg3, val);
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
        case (3):
            eg_set_decay_time_msec(&sds->m_eg3, val);
            break;
        }
    }
    else
        printf("Val has to be between %d and %d\n", EG_MINTIME_MS,
               EG_MAXTIME_MS);
}

void synthdrum_set_eg_sustain_ms(synthdrum_sequencer *sds, int eg_num,
                                 double val)
{
    if (val >= 0 && val <= 5000)
    {
        int samples_val = SAMPLE_RATE / 1000. * val;
        switch (eg_num)
        {
        case (1):
            sds->eg1_sustain_len_in_samples = samples_val;
            break;
        case (2):
            sds->eg2_sustain_len_in_samples = samples_val;
            break;
        case (3):
            sds->eg3_sustain_len_in_samples = samples_val;
            break;
        }
    }
    else
        printf("Val has to be between %d and %d\n", EG_MINTIME_MS,
               EG_MAXTIME_MS);
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
        case (3):
            eg_set_release_time_msec(&sds->m_eg3, val);
            break;
        }
    }
    else
        printf("Val has to be between %d and %d\n", EG_MINTIME_MS,
               EG_MAXTIME_MS);
}

void synthdrum_set_eg2_osc_intensity(synthdrum_sequencer *sds, double val)
{
    if (val >= 0 && val <= 1)
        sds->eg2_osc2_intensity = val;
    else
        printf("Val has to be between 0 and 1\n");
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

int sds_get_num_tracks(void *self)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer *)self;
    return sds->m_seq.num_patterns;
}

void sds_make_active_track(void *self, int track_num)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer *)self;
    sds->m_seq.cur_pattern = track_num;
}

void synthdrum_set_distortion_threshold(synthdrum_sequencer *sds, double val)
{
    printf("setting distortion to %f\n", val);
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
