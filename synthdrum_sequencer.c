#include "synthdrum_sequencer.h"
#include "mixer.h"
#include "sequencer_utils.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

extern mixer *mixr;
extern char *state_strings;

const int OSC1_SUSTAIN_MS = 10;
const int OSC2_SUSTAIN_MS = 0;
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
    sds->m_eg1.m_attack_time_msec = 10;
    sds->m_eg1.m_decay_time_msec = 0;
    sds->m_eg1.m_release_time_msec = 175;

    envelope_generator_init(&sds->m_eg2);
    sds->m_eg2.m_attack_time_msec = 2;
    sds->m_eg2.m_decay_time_msec = 2;
    sds->m_eg2.m_release_time_msec = 2;

    osc_new_settings(&sds->m_osc1.osc);
    qb_set_soundgenerator_interface(&sds->m_osc1);
    sds->m_osc1.osc.m_waveform = SINE;
    sds->m_osc1.osc.m_osc_fo = DEFAULT_PITCH;
    sds->osc1_sustain_len_in_samples = SAMPLE_RATE / 1000. * OSC1_SUSTAIN_MS;
    sds->osc1_sustain_counter = 0;
    sds->osc1_amp = 1;
    sds->eg1_osc1_intensity = 1;

    osc_new_settings(&sds->m_osc2.osc);
    qb_set_soundgenerator_interface(&sds->m_osc2);
    sds->m_osc2.osc.m_waveform = NOISE;
    sds->osc2_sustain_len_in_samples = SAMPLE_RATE / 1000. * OSC2_SUSTAIN_MS;
    sds->osc2_sustain_counter = 0;
    sds->osc2_amp = 0.0;

    filter_moog_init(&sds->m_filter);

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
        "[SYNTHDRUM] Type: %s Vol: %.2f Envelope STATE: %d SustainOverride: %d"
        "\n      Osc1 Waveform: %d Fo: %.0f Pitch Env A:%.2f D:%.2f S:%.2f "
        "R:%.2f "
        "Distortion_Threshold: %.2f\n",
        sds->drumtype == KICK ? "drum" : "snare", sds->vol, sds->m_eg1.m_state,
        sds->m_eg1.m_sustain_override, sds->m_osc1.osc.m_waveform,
        sds->m_osc1.osc.m_osc_fo, sds->m_eg1.m_attack_time_msec,
        sds->m_eg1.m_decay_time_msec,
        sds->osc1_sustain_len_in_samples / (SAMPLE_RATE / 1000.),
        sds->m_eg1.m_release_time_msec, sds->m_distortion_threshold);

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
        if (sds->m_seq.patterns[sds->m_seq.cur_pattern])
            sds_trigger(sds);
    }
    seq_tick(&sds->m_seq);

    // END POSITIONAL /////////////////////////////////////////

    if (sds->m_eg1.m_state == SUSTAIN) {
        sds->osc1_sustain_counter++;
        if (sds->osc1_sustain_counter >= sds->osc1_sustain_len_in_samples) {
            sds->osc1_sustain_counter = 0;
            sds->m_eg1.m_state = RELEASE;
        }
    }
    if (sds->m_eg2.m_state == SUSTAIN) {
        sds->osc2_sustain_counter++;
        if (sds->osc2_sustain_counter >= sds->osc2_sustain_len_in_samples) {
            sds->osc2_sustain_counter = 0;
            sds->m_eg2.m_state = RELEASE;
        }
    }

    double eg1_biased = 0.;
    double eg1 = eg_do_envelope(&sds->m_eg1, &eg1_biased);

    double eg1_osc_mod =
        sds->eg1_osc1_intensity * OSC_FO_MOD_RANGE * eg1_biased;
    sds->m_osc1.osc.m_fo_mod = eg1_osc_mod;

    osc_update(&sds->m_osc1.osc);
    double osc1 = qb_do_oscillate(&sds->m_osc1.osc, NULL);

    // noise for initial hit
    double eg2 = eg_do_envelope(&sds->m_eg2, NULL);
    osc_update(&sds->m_osc2.osc);
    double osc2 = qb_do_oscillate(&sds->m_osc2.osc, NULL);

    val = (eg1 * osc1) * 2; //+ (eg2 * osc2 * sds->osc2_amp);

    return val * sds->vol;
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
    sds->osc1_sustain_counter = 0;

    osc_reset(&sds->m_osc2.osc);
    sds->m_osc2.osc.m_note_on = true;
    eg_start_eg(&sds->m_eg2);
    sds->osc2_sustain_counter = 0;
}

void sds_stop(synthdrum_sequencer *sds)
{
    sds->m_osc1.osc.m_note_on = false;
    sds->m_osc2.osc.m_note_on = false;
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
                sds->osc1_sustain_len_in_samples =
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
                        " %f"    // osc1_sustain_len_in_samples
                        " %f"    // eg1_osc1_intensity
                        " %f"    // osc2_amp
                        " %f"    // m_osc2.osc.m_osc_fo
                        " %d"    // m_osc2.osc.m_waveform
                        " %f"    // osc2_sustain_len_in_samples
                        " %f"    // osc3_amp
                        " %f"    // m_osc3.osc.m_osc_fo
                        " %d"    // m_osc3.osc.m_waveform
                        " %f"    // osc3_sustain_len_in_samples
                        " %f"    // m_eg1.m_attack_time_msec
                        " %f"    // m_eg1.m_decay_time_msec
                        " %f"    // m_eg1.m_release_time_msec
                        " %f"    // m_eg2.m_attack_time_msec
                        " %f"    // m_eg2.m_decay_time_msec
                        " %f\n", // m_eg2.m_release_time_msec
            sds->m_patch_name, sds->vol, sds->drumtype, sds->osc1_amp,
            sds->m_osc1.osc.m_osc_fo, sds->m_osc1.osc.m_waveform,
            sds->osc1_sustain_len_in_samples, sds->eg1_osc1_intensity,
            sds->osc2_amp, sds->m_osc2.osc.m_osc_fo, sds->m_osc2.osc.m_waveform,
            sds->osc2_sustain_len_in_samples, sds->osc3_amp,
            sds->m_osc3.osc.m_osc_fo, sds->m_osc3.osc.m_waveform,
            sds->osc3_sustain_len_in_samples, sds->m_eg1.m_attack_time_msec,
            sds->m_eg1.m_decay_time_msec, sds->m_eg1.m_release_time_msec,
            sds->m_eg2.m_attack_time_msec, sds->m_eg2.m_decay_time_msec,
            sds->m_eg2.m_release_time_msec);

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
                line, "%s"       // m_patch_name
                      "  %lf"    // vol
                      "  %d"     // drumtype
                      "  %lf"    // osc1_amp
                      "  %lf"    // m_osc1.osc.m_osc_fo
                      "  %d"     // m_osc1.osc.m_waveform
                      "  %lf"    // osc1_sustain_len_in_samples
                      "  %lf"    // eg1_osc1_intensity
                      "  %lf"    // osc2_amp
                      "  %lf"    // m_osc2.osc.m_osc_fo
                      "  %d"     // m_osc2.osc.m_waveform
                      "  %lf"    // osc2_sustain_len_in_samples
                      "  %lf"    // osc3_amp
                      "  %lf"    // m_osc3.osc.m_osc_fo
                      "  %d"     // m_osc3.osc.m_waveform
                      "  %lf"    // osc3_sustain_len_in_samples
                      "  %lf"    // m_eg1.m_attack_time_msec
                      "  %lf"    // m_eg1.m_decay_time_msec
                      "  %lf"    // m_eg1.m_release_time_msec
                      "  %lf"    // m_eg2.m_attack_time_msec
                      "  %lf"    // m_eg2.m_decay_time_msec
                      "  %lf\n", // m_eg2.m_release_time_msec
                sds->m_patch_name, &sds->vol, &sds->drumtype, &sds->osc1_amp,
                &sds->m_osc1.osc.m_osc_fo, &sds->m_osc1.osc.m_waveform,
                &sds->osc1_sustain_len_in_samples, &sds->eg1_osc1_intensity,
                &sds->osc2_amp, &sds->m_osc2.osc.m_osc_fo,
                &sds->m_osc2.osc.m_waveform, &sds->osc2_sustain_len_in_samples,
                &sds->osc3_amp, &sds->m_osc3.osc.m_osc_fo,
                &sds->m_osc3.osc.m_waveform, &sds->osc3_sustain_len_in_samples,
                &sds->m_eg1.m_attack_time_msec, &sds->m_eg1.m_decay_time_msec,
                &sds->m_eg1.m_release_time_msec, &sds->m_eg2.m_attack_time_msec,
                &sds->m_eg2.m_decay_time_msec, &sds->m_eg2.m_release_time_msec);
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
    char *item, *last_item;
    char const *sep = "::";
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
}
