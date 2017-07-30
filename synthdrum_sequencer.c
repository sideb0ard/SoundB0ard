#include "synthdrum_sequencer.h"
#include "mixer.h"
#include "sequencer_utils.h"
#include "utils.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

extern mixer *mixr;
extern char *state_strings;

const int EG1_SUSTAIN_MS = 0;
const int EG2_SUSTAIN_MS = 0;
const int EG3_SUSTAIN_MS = 0;
const double DEFAULT_PITCH = 97;

synthdrum_sequencer *new_synthdrum_seq()
{
    printf("New Drum Synth!\n");
    synthdrum_sequencer *sds = calloc(1, sizeof(synthdrum_sequencer));
    seq_init(&sds->m_seq);
    sds->m_pitch = DEFAULT_PITCH;

    sds->vol = 1;
    sds->started = false;
    sds->midi_controller_mode =
        0; // TODO enum or sumthing - for the moment just two modes
    for (int i = 0; i < SEQUENCER_PATTERN_LEN; i++) {
        sds->metadata[i].played = 0.0;
        sds->metadata[i].playing = 0.0;
    }

    // pitch envelope
    envelope_generator_init(&sds->m_eg1);
    sds->m_eg1.m_attack_time_msec = 2;
    sds->m_eg1.m_decay_time_msec = 0;
    sds->m_eg1.m_release_time_msec = 5;
    sds->eg1_sustain_len_in_samples = SAMPLE_RATE / 1000. * EG1_SUSTAIN_MS;
    sds->eg1_sustain_counter = 0;

    envelope_generator_init(&sds->m_eg2);
    sds->m_eg2.m_attack_time_msec = 2;
    sds->m_eg2.m_decay_time_msec = 50;
    sds->m_eg2.m_release_time_msec = 2;
    sds->eg2_sustain_len_in_samples = SAMPLE_RATE / 1000. * EG2_SUSTAIN_MS;
    sds->eg2_sustain_counter = 0;
    sds->eg2_osc2_intensity = 1;

    envelope_generator_init(&sds->m_eg3);
    sds->m_eg3.m_attack_time_msec = 2;
    sds->m_eg3.m_decay_time_msec = 50;
    sds->m_eg3.m_release_time_msec = 2;
    sds->eg3_sustain_len_in_samples = SAMPLE_RATE / 1000. * EG3_SUSTAIN_MS;
    sds->eg3_sustain_counter = 0;

    osc_new_settings(&sds->m_osc1.osc);
    qb_set_soundgenerator_interface(&sds->m_osc1);
    sds->m_osc1.osc.m_waveform = NOISE;
    sds->m_osc1.osc.m_osc_fo = DEFAULT_PITCH;
    sds->osc1_amp = 1;

    osc_new_settings(&sds->m_osc2.osc);
    qb_set_soundgenerator_interface(&sds->m_osc2);
    sds->m_osc2.osc.m_waveform = SINE;
    sds->m_osc2.osc.m_osc_fo = 58;
    sds->osc2_amp = 1.0;

    // filter_moog_init(&sds->m_filter);

    sds->sg.gennext = &sds_gennext;
    sds->sg.status = &sds_status;
    sds->sg.getvol = &sds_getvol;
    sds->sg.setvol = &sds_setvol;
    sds->sg.type = SYNTHDRUM_TYPE;

    return sds;
}

void sds_status(void *self, wchar_t *ss)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer *)self;
    swprintf(
        ss, MAX_PS_STRING_SZ, WANSI_COLOR_GREEN
        "[SYNTHDRUM] Type: %s Vol: %.2f AmpEnv STATE: %d PitchEnv STATE: %d "
        "\n      Osc1 osc1_wav:%d osc1_fo:%.0f eg1_attack:%.2f eg1_decay:%.2f "
        "eg1_sustain_ms:%.2f eg1_release:%.2f "
        "\n      Osc2 osc2_wav:%d osc2_fo:%.0f eg2_attack:%.2f eg2_decay:%.2f "
        "eg2_sustain_ms:%.2f eg2_release:%.2f eg2_osc2_int:%.2f"
        "\n      eg3_attack:%.2f eg3_decay:%.2f eg3_sustain_ms:%.2f "
        "eg3_release:%.2f",
        sds->drumtype == KICK ? "drum" : "snare", sds->vol, sds->m_eg1.m_state,
        sds->m_eg2.m_state, sds->m_osc1.osc.m_waveform,
        sds->m_osc1.osc.m_osc_fo, sds->m_eg1.m_attack_time_msec,
        sds->m_eg1.m_decay_time_msec,
        sds->eg1_sustain_len_in_samples / (SAMPLE_RATE / 1000.),
        sds->m_eg1.m_release_time_msec,

        sds->m_osc2.osc.m_waveform, sds->m_osc2.osc.m_fo,
        sds->m_eg2.m_attack_time_msec, sds->m_eg2.m_decay_time_msec,
        sds->eg2_sustain_len_in_samples / (SAMPLE_RATE / 1000.),
        sds->m_eg2.m_release_time_msec, sds->eg2_osc2_intensity,

        sds->m_eg3.m_attack_time_msec, sds->m_eg3.m_decay_time_msec,
        sds->eg3_sustain_len_in_samples / (SAMPLE_RATE / 1000.),
        sds->m_eg3.m_release_time_msec);

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

double sds_gennext(void *self)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer *)self;
    double val = 0.0;

    // POSITIONAL
    int idx = mixr->midi_tick % PPBAR;

    if (!sds->started) {
        if (idx == 0)
            sds->started = true;
        else
            return val;
    }

    if (mixr->is_midi_tick) {
        if (sds->m_seq.patterns[sds->m_seq.cur_pattern][idx])
            sds_trigger(sds);
    }
    seq_tick(&sds->m_seq);

    // END POSITIONAL /////////////////////////////////////////

    if (sds->m_eg1.m_state == SUSTAIN) {
        sds->eg1_sustain_counter++;
        if (sds->eg1_sustain_counter >= sds->eg1_sustain_len_in_samples) {
            sds->eg1_sustain_counter = 0;
            sds->m_eg1.m_state = RELEASE;
        }
    }
    if (sds->m_eg2.m_state == SUSTAIN) {
        sds->eg2_sustain_counter++;
        if (sds->eg2_sustain_counter >= sds->eg2_sustain_len_in_samples) {
            sds->eg2_sustain_counter = 0;
            sds->m_eg2.m_state = RELEASE;
        }
    }
    if (sds->m_eg3.m_state == SUSTAIN) {
        sds->eg3_sustain_counter++;
        if (sds->eg3_sustain_counter >= sds->eg3_sustain_len_in_samples) {
            sds->eg3_sustain_counter = 0;
            sds->m_eg3.m_state = RELEASE;
        }
    }

    double eg1_out = eg_do_envelope(&sds->m_eg1, NULL);
    osc_update(&sds->m_osc1.osc);
    double osc1_out = qb_do_oscillate(&sds->m_osc1.osc, NULL) * eg1_out;

    double eg2_out = eg_do_envelope(&sds->m_eg2, NULL);
    double eg2_osc_mod = sds->eg2_osc2_intensity * OSC_FO_MOD_RANGE * eg2_out;
    sds->m_osc2.osc.m_fo_mod = eg2_osc_mod;
    osc_update(&sds->m_osc2.osc);
    double osc2_out = qb_do_oscillate(&sds->m_osc2.osc, NULL);

    val = osc1_out + osc2_out;

    return val * sds->vol * eg_do_envelope(&sds->m_eg3, NULL);
    ;
    // moog_update((filter *)&sds->m_filter);
    // double filter_out =
    //  moog_gennext((filter *)&sds->m_filter, val);

    // return filter_out * sds->vol;
}

double sds_getvol(void *self)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer *)self;
    return sds->vol;
}

void sds_trigger(synthdrum_sequencer *sds)
{
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

void sds_parse_midi(synthdrum_sequencer *sds, int status, int data1, int data2)
{
    // switch (ev->event_type) {
    // case (144): { // Hex 0x80
    //    ms->m_last_midi_note = ev->data1;
    //    minisynth_midi_note_on(ms, ev->data1, ev->data2);
    //    break;
    if (status == 144) // note on
    {
        printf("PITCH!\n");
        sds->m_pitch = data1;
        sds->m_osc1.osc.m_osc_fo = data1;
    }
    else if (status == 176) // control
    {
        printf("SYNTHDRUMMIDI!\n");
        double scaley_val = 0.0;
        switch (data1) {
        case 9:
            printf("9\n");
            break;
        case 10:
            printf("10\n");
            break;
        case 11:
            printf("11\n");
            break;
        case 12:
            printf("12\n");
            break;
        case 13:
            printf("13\n");
            break;
        case 14:
            printf("14\n");
            break;
        case 15:
            printf("15\n");
            break;
        case 16:
            printf("Toggle! MIDI Knob Modee!\n");
            sds->midi_controller_mode = 1 - sds->midi_controller_mode;
            break;
        /// BANK B on MPK Mini MKII
        case 17:
            printf("Delay Mode! Mode\n");
            break;
        case 18:
            printf("Sustain Override! Mode\n");
            break;
        case 19:
            printf("19! \n");
            break;
        case 20:
            printf("20! Mode\n");
            break;
        case 21:
            printf("21! MIDI Mode\n");
            break;
        case 22:
            printf("22! MIDI Mode\n");
            break;
        case 23:
            printf("23! MIDI Mode\n");
            break;
        case 24:
            printf("24! MIDI Mode\n");
            break;
        case 1:
            if (sds->midi_controller_mode == 0) {
                scaley_val = scaleybum(0, 128, 48, 107, data2);
                printf("TUNE! %f\n", scaley_val);
                sds->m_osc1.osc.m_osc_fo = scaley_val;
            }
            else {
                scaley_val = scaleybum(0, 127, 2, 20, data2);
                eg_set_attack_time_msec(&sds->m_eg1, scaley_val);
                printf("ENV1 ATTACK! %f\n", sds->m_eg1.m_attack_time_msec);
            }
            break;
        case 2:
            if (sds->midi_controller_mode == 0) {
                scaley_val = scaleybum(0, 127, 0, 1, data2);
                sds->vol = scaley_val;
                printf("LEVEL!! %f\n", scaley_val);
            }
            else {
            }
            break;
        case 3:
            if (sds->midi_controller_mode == 0) {
                scaley_val = scaleybum(0, 127, 1, 50, data2);
                eg_set_attack_time_msec(&sds->m_eg1, scaley_val);
                printf("ATTACK!! %f\n", scaley_val);
            }
            else {
                scaley_val = scaleybum(0, 127, 0, 500, data2);
                sds->eg1_sustain_len_in_samples =
                    SAMPLE_RATE / 1000. * scaley_val;
            }
            break;
        case 4:
            if (sds->midi_controller_mode == 0) {
                scaley_val = scaleybum(0, 127, 1, 100, data2);
                eg_set_decay_time_msec(&sds->m_eg1, scaley_val);
                eg_set_release_time_msec(&sds->m_eg1, scaley_val);
                printf("DECAY/RELEASE!! %f\n", scaley_val);
            }
            else {
                scaley_val =
                    scaleybum(0, 127, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
                eg_set_release_time_msec(&sds->m_eg1, scaley_val);
                printf("ENV1 RELEASE!! %f\n", sds->m_eg1.m_release_time_msec);
                // scaley_val = scaleybum(0, 127, FILTER_FC_MIN, FILTER_FC_MAX,
                // data2);
                // sds->m_filter.f.m_fc = scaley_val;
                // printf("FILTER FREQ CUTOFF %f!\n", m_filter.f.m_fc);
                // printf("FILTER Qviiity !\n");
                // scaley_val = scaleybum(0, 127, 1, 10, data2);
                // sds->m_filter.f.m_q = scaley_val;
            }
            break;
        case 5:
            if (sds->midi_controller_mode == 0) {
                scaley_val = floor(scaleybum(0, 127, 0, 8, data2));
                printf("OSC TYPE! %f\n", scaley_val);
                sds->m_osc1.osc.m_waveform = scaley_val;
            }
            else {
            }
            break;
        case 6:
            if (sds->midi_controller_mode == 0) {
                scaley_val = scaleybum(0, 127, 0, 1, data2);
                sds->osc2_amp = scaley_val;
                printf("OSC2 AMp!! %f\n", scaley_val);
            }
            else {
            }
            break;
        case 7:
            if (sds->midi_controller_mode == 0) {
                scaley_val = scaleybum(0, 127, 0, 220, data2);
                sds->m_eg2.m_decay_time_msec = scaley_val;
                sds->m_eg2.m_release_time_msec = scaley_val;
                printf("ENV2 DECAY/RELEASE!! %f\n",
                       sds->m_eg2.m_release_time_msec);
            }
            else {
            }
            break;
        case 8:
            if (sds->midi_controller_mode == 0) {
                printf("DISTORTION!!\n");
                scaley_val = scaleybum(0, 127, 100, 0.01, data2);
                sds->m_distortion_threshold = scaley_val;
            }
            else {
            }
            break;
        default:
            printf("SOMthing else\n");
        }
    }
}

bool synthdrum_save_patch(synthdrum_sequencer *sds, char *name)
{
    if (strlen(name) == 0) {
        printf("Play tha game, pal, need a name to save yer synthdrum settings "
               "with\n");
        return false;
    }
    strncpy(sds->m_patch_name, name, 511);
    printf("Saving '%s' settings for Drumsynth to file %s\n", name,
           DRUMSYNTH_SAVED_SETUPS_FILENAME);
    FILE *filetosave = fopen(DRUMSYNTH_SAVED_SETUPS_FILENAME, "a");

    fprintf(filetosave, "%s"     // m_patch_name
                        " %f"    // vol
                        " %d"    // drumtype
                        " %f"    // osc1_amp
                        " %f"    // m_osc1.osc.m_osc_fo
                        " %d"    // m_osc1.osc.m_waveform
                        " %f"    // osc2_amp
                        " %f"    // m_osc2.osc.m_osc_fo
                        " %d"    // m_osc2.osc.m_waveform
                        " %f"    // m_eg1.m_attack_time_msec
                        " %f"    // m_eg1.m_decay_time_msec
                        " %f"    // m_eg1.m_release_time_msec
                        " %f"    // eg1_sustain_len_in_samples
                        " %f"    // m_eg2.m_attack_time_msec
                        " %f"    // m_eg2.m_decay_time_msec
                        " %f"    // m_eg2.m_release_time_msec
                        " %f"    // eg2_osc2_intensity
                        " %f"    // eg2_sustain_len_in_samples
                        " %f"    // m_eg3.m_attack_time_msec
                        " %f"    // m_eg3.m_decay_time_msec
                        " %f"    // m_eg3.m_release_time_msec
                        " %f\n", // eg3_sustain_len_in_samples
            sds->m_patch_name, sds->vol, sds->drumtype, sds->osc1_amp,
            sds->m_osc1.osc.m_osc_fo, sds->m_osc1.osc.m_waveform, sds->osc2_amp,
            sds->m_osc2.osc.m_osc_fo, sds->m_osc2.osc.m_waveform,

            sds->m_eg1.m_attack_time_msec, sds->m_eg1.m_decay_time_msec,
            sds->m_eg1.m_release_time_msec, sds->eg1_sustain_len_in_samples,

            sds->m_eg2.m_attack_time_msec, sds->m_eg2.m_decay_time_msec,
            sds->m_eg2.m_release_time_msec, sds->eg2_sustain_len_in_samples,
            sds->eg2_osc2_intensity,

            sds->m_eg3.m_attack_time_msec, sds->m_eg3.m_decay_time_msec,
            sds->m_eg3.m_release_time_msec, sds->eg3_sustain_len_in_samples

            );

    fclose(filetosave);
    return true;
}

bool synthdrum_open_patch(synthdrum_sequencer *sds, char *name)
{
    FILE *fp = fopen(DRUMSYNTH_SAVED_SETUPS_FILENAME, "r");
    if (fp == NULL) {
        printf("Dingie!\n");
        return false;
    }
    char line[256];
    char patch_name[52];
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
        sscanf(line, "%s", patch_name);
        if (strncmp(patch_name, name, 255) == 0) {
            printf("MATCH PATCH NAME %s\n", patch_name);
            printf("BEFORE OSC_FO %f\n", sds->m_osc1.osc.m_osc_fo);
            int num = sscanf(
                line, "%s"      // m_patch_name
                      " %lf"    // vol
                      " %d"     // drumtype
                      " %lf"    // osc1_amp
                      " %lf"    // m_osc1.osc.m_osc_fo
                      " %d"     // m_osc1.osc.m_waveform
                      " %lf"    // osc2_amp
                      " %lf"    // m_osc2.osc.m_osc_fo
                      " %d"     // m_osc2.osc.m_waveform
                      " %lf"    // m_eg1.m_attack_time_msec
                      " %lf"    // m_eg1.m_decay_time_msec
                      " %lf"    // m_eg1.m_release_time_msec
                      " %lf"    // eg1_sustain_len_in_samples
                      " %lf"    // m_eg2.m_attack_time_msec
                      " %lf"    // m_eg2.m_decay_time_msec
                      " %lf"    // m_eg2.m_release_time_msec
                      " %lf"    // eg2_osc2_intensity
                      " %lf"    // eg2_sustain_len_in_samples
                      " %lf"    // m_eg3.m_attack_time_msec
                      " %lf"    // m_eg3.m_decay_time_msec
                      " %lf"    // m_eg3.m_release_time_msec
                      " %lf\n", // eg3_sustain_len_in_samples
                sds->m_patch_name, &sds->vol, &sds->drumtype, &sds->osc1_amp,
                &sds->m_osc1.osc.m_osc_fo, &sds->m_osc1.osc.m_waveform,
                &sds->osc2_amp, &sds->m_osc2.osc.m_osc_fo,
                &sds->m_osc2.osc.m_waveform,

                &sds->m_eg1.m_attack_time_msec, &sds->m_eg1.m_decay_time_msec,
                &sds->m_eg1.m_release_time_msec,
                &sds->eg1_sustain_len_in_samples,

                &sds->m_eg2.m_attack_time_msec, &sds->m_eg2.m_decay_time_msec,
                &sds->m_eg2.m_release_time_msec,
                &sds->eg2_sustain_len_in_samples, &sds->eg2_osc2_intensity,

                &sds->m_eg3.m_attack_time_msec, &sds->m_eg3.m_decay_time_msec,
                &sds->m_eg3.m_release_time_msec,
                &sds->eg3_sustain_len_in_samples

                );

            //          "  %lf"    // vol
            //          "  %d"     // drumtype
            //          "  %lf"    // osc1_amp
            //          "  %lf"    // m_osc1.osc.m_osc_fo
            //          "  %d"     // m_osc1.osc.m_waveform
            //          "  %lf"    // osc1_sustain_len_in_samples
            //          "  %lf"    // eg2_osc2_intensity
            //          "  %lf"    // osc2_amp
            //          "  %lf"    // m_osc2.osc.m_osc_fo
            //          "  %d"     // m_osc2.osc.m_waveform
            //          "  %lf"    // osc2_sustain_len_in_samples
            //          "  %lf"    // m_eg1.m_attack_time_msec
            //          "  %lf"    // m_eg1.m_decay_time_msec
            //          "  %lf"    // m_eg1.m_release_time_msec
            //          "  %lf"    // m_eg2.m_attack_time_msec
            //          "  %lf"    // m_eg2.m_decay_time_msec
            //          "  %lf"    // m_eg2.m_release_time_msec
            //          "  %lf"    // m_eg3.m_attack_time_msec
            //          "  %lf"    // m_eg3.m_decay_time_msec
            //          "  %lf\n", // m_eg3.m_release_time_msec
            //    sds->m_patch_name, &sds->vol, &sds->drumtype, &sds->osc1_amp,
            //    &sds->m_osc1.osc.m_osc_fo, &sds->m_osc1.osc.m_waveform,
            //    &sds->osc1_sustain_len_in_samples, &sds->eg2_osc2_intensity,
            //    &sds->osc2_amp, &sds->m_osc2.osc.m_osc_fo,
            //    &sds->m_osc2.osc.m_waveform,
            //    &sds->osc2_sustain_len_in_samples,
            //    &sds->m_eg1.m_attack_time_msec,
            //    &sds->m_eg1.m_decay_time_msec,
            //    &sds->m_eg1.m_release_time_msec,
            //    &sds->m_eg2.m_attack_time_msec,
            //    &sds->m_eg2.m_decay_time_msec,
            //    &sds->m_eg2.m_release_time_msec,
            //    &sds->m_eg3.m_attack_time_msec,
            //    &sds->m_eg3.m_decay_time_msec,
            //    &sds->m_eg3.m_release_time_msec
            //    );
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
    if (fp == NULL) {
        printf("Dingie!\n");
        return false;
    }
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
    }
    fclose(fp);
    return true;
}

void synthdrum_del_self(synthdrum_sequencer *sds)
{
    printf("Deleting Synthdrum self\n");
    free(sds);
}
