#include "synthdrum_sequencer.h"
#include "mixer.h"
#include "sequencer_utils.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

extern mixer *mixr;

const int OSC1_SUSTAIN_MS = 15;
const int OSC2_SUSTAIN_MS = 150;

synthdrum_sequencer *new_synthdrum_seq(int drumtype)
{
    printf("New Drum Synth!\n");
    synthdrum_sequencer *sds = calloc(1, sizeof(synthdrum_sequencer));
    seq_init(&sds->m_seq);

    sds->drumtype = drumtype;
    sds->vol = 0.7;
    sds->started = false;
    sds->midi_controller_mode =
        0; // TODO enum or sumthing - for the moment just two modes
    for (int i = 0; i < SEQUENCER_PATTERN_LEN; i++) {
        sds->metadata[i].played = 0.0;
        sds->metadata[i].playing = 0.0;
    }

    envelope_generator_init(&sds->m_eg1);
    sds->m_eg1.m_attack_time_msec = 2;
    sds->m_eg1.m_decay_time_msec = 2;
    sds->m_eg1.m_release_time_msec = 2;

    envelope_generator_init(&sds->m_eg2);

    osc_new_settings(&sds->m_osc1.osc);
    qb_set_soundgenerator_interface(&sds->m_osc1);
    sds->m_osc1.osc.m_waveform = NOISE;
    sds->osc1_sustain_len_in_samples =
        SAMPLE_RATE / 1000. * (OSC1_SUSTAIN_MS + sds->m_eg1.m_attack_time_msec +
                               sds->m_eg1.m_decay_time_msec);
    sds->osc1_sustain_counter = 0;
    sds->osc1_amp = 0.1;

    osc_new_settings(&sds->m_osc2.osc);
    qb_set_soundgenerator_interface(&sds->m_osc2);
    sds->m_osc2.osc.m_waveform = SINE;
    sds->m_osc2.osc.m_osc_fo = 200;
    sds->osc2_sustain_len_in_samples =
        SAMPLE_RATE / 1000. * (OSC2_SUSTAIN_MS + sds->m_eg2.m_attack_time_msec +
                               sds->m_eg2.m_decay_time_msec);
    sds->osc2_sustain_counter = 0;
    sds->osc2_amp = 0.7;

    osc_new_settings(&sds->m_osc3.osc);
    qb_set_soundgenerator_interface(&sds->m_osc3);
    sds->m_osc3.osc.m_waveform = SINE;
    sds->m_osc3.osc.m_osc_fo = 147;
    sds->osc3_sustain_len_in_samples =
        SAMPLE_RATE / 1000. *
        (sds->m_eg2.m_attack_time_msec + sds->m_eg2.m_decay_time_msec);
    sds->osc3_sustain_counter = 0;
    sds->osc3_amp = 0.2;

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
    swprintf(ss, MAX_PS_STRING_SZ,
             WANSI_COLOR_GREEN "[SYNTHDRUM] Type: %s Vol: %.2lf",
             sds->drumtype == KICK ? "drum" : "snare", sds->vol);

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

    int step_seq_idx = mixr->sixteenth_note_tick % SEQUENCER_PATTERN_LEN;

    if (!sds->started) {
        if (step_seq_idx == 0)
            sds->started = true;
        else
            return val;
    }

    int bit_position = 1 << (15 - step_seq_idx);
    if ((sds->m_seq.patterns[sds->m_seq.cur_pattern] & bit_position) &&
        mixr->is_sixteenth) {
        sds_trigger(sds);
    }
    seq_tick(&sds->m_seq);

    if (sds->osc1_sustain_counter >= sds->osc1_sustain_len_in_samples)
        sds->m_eg1.m_state = RELEASE;

    if (sds->osc2_sustain_counter >= sds->osc2_sustain_len_in_samples)
        sds->m_eg2.m_state = RELEASE;

    eg_update(&sds->m_eg1);
    double eg1 = eg_do_envelope(&sds->m_eg1, NULL);
    sds->osc1_sustain_counter++;

    eg_update(&sds->m_eg2);
    double eg2 = eg_do_envelope(&sds->m_eg2, NULL);
    sds->osc2_sustain_counter++;

    double scaled_eg1 = scaleybum(0, 1.0, -1.0, 1.0, eg2);
    sds->m_osc2.osc.m_fo_mod = scaled_eg1;

    osc_update(&sds->m_osc1.osc, "NOISEOSC");
    osc_update(&sds->m_osc2.osc, "SINEOSC");

    double osc1 = qb_do_oscillate(&sds->m_osc1.osc, NULL);
    double osc2 = qb_do_oscillate(&sds->m_osc2.osc, NULL);

    if (mixr->debug_mode)
        if (osc1 > 0.0 || osc2 > 0.0)
            printf("oSC1: %f OSC2: %f\n", osc1, osc2);

    // val = eg2 * (eg1*osc1*sds->osc1_amp + osc2*sds->osc2_amp);
    val = eg2 * osc2 * sds->osc2_amp;

    // moog_update((filter *)&sds->m_filter);
    // double filter_out =
    //    moog_gennext((filter *)&sds->m_filter, val);
    // if (mixr->debug_mode)
    //    printf("FILTEROUOOT %f\n", filter_out);

    // return filter_out * sds->vol;
    return val * sds->vol;
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

void sds_parse_midi(synthdrum_sequencer *sds, int data1, int data2)
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
            scaley_val = scaleybum(1, 128, 0.0, 1.0, data2);
            printf("VOLUME!\n");
            sds->vol = scaley_val;
        }
        else {
            scaley_val = scaleybum(1, 128, 0.0, 1.0, data2);
            printf("OSC1 AMP!\n");
            sds->osc1_amp = scaley_val;
        }
        break;
    case 2:
        if (sds->midi_controller_mode == 0) {
            scaley_val = scaleybum(1, 128, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
            sds->m_eg1.m_attack_time_msec = scaley_val;
            printf("ENV1 ATTACK! %f\n", sds->m_eg1.m_attack_time_msec);
        }
        else {
            scaley_val = scaleybum(1, 128, 0.0, 1.0, data2);
            printf("OSC2 AMP!\n");
            sds->osc2_amp = scaley_val;
        }
        break;
    case 3:
        if (sds->midi_controller_mode == 0) {
            scaley_val = scaleybum(1, 128, 5, 500, data2);
            sds->osc1_sustain_len_in_samples =
                SAMPLE_RATE / 1000. *
                (scaley_val + +sds->m_eg1.m_attack_time_msec +
                 sds->m_eg1.m_decay_time_msec);
            ;
            printf("OSC1 SUSTAIN!! %f\n", scaley_val);
        }
        else {
        }
        break;
    case 4:
        if (sds->midi_controller_mode == 0) {
            scaley_val = scaleybum(1, 128, 10, 220, data2);
            sds->m_eg1.m_decay_time_msec = scaley_val;
            sds->m_eg1.m_release_time_msec = scaley_val;
            printf("ENV1 DECAY/RELEASE!! %f\n", sds->m_eg1.m_release_time_msec);
        }
        else {
        }
        break;
    case 5:
        if (sds->midi_controller_mode == 0) {
            scaley_val = scaleybum(0, 128, OSC_FO_MIN, 300, data2);
            printf("OSC2 freQ! %f\n", scaley_val);
            sds->m_osc2.osc.m_osc_fo = scaley_val;
        }
        else {
        }
        break;
    case 6:
        if (sds->midi_controller_mode == 0) {
            scaley_val = scaleybum(1, 128, EG_MINTIME_MS, EG_MAXTIME_MS, data2);
            sds->m_eg2.m_attack_time_msec = scaley_val;
            printf("ENV2 ATTACK! %f\n", sds->m_eg2.m_attack_time_msec);
        }
        else {
        }
        break;
    case 7:
        if (sds->midi_controller_mode == 0) {
            scaley_val = scaleybum(1, 128, 5, 500, data2);
            sds->osc2_sustain_len_in_samples =
                SAMPLE_RATE / 1000. *
                (scaley_val + +sds->m_eg2.m_attack_time_msec +
                 sds->m_eg2.m_decay_time_msec);
            printf("OSC2 SUSTAIN!! %f\n", scaley_val);
        }
        else {
            printf("FILTER Qviiity !\n");
            scaley_val = scaleybum(1, 128, 1, 10, data2);
            sds->m_filter.f.m_q = scaley_val;
        }
        break;
    case 8:
        if (sds->midi_controller_mode == 0) {
            scaley_val = scaleybum(1, 128, 10, 220, data2);
            sds->m_eg2.m_decay_time_msec = scaley_val;
            sds->m_eg2.m_release_time_msec = scaley_val;
            printf("ENV2 DECAY/RELEASE!! %f\n", sds->m_eg2.m_release_time_msec);
        }
        else {
            printf("FILTER FREQ CUTOFF!\n");
            scaley_val = scaleybum(1, 128, FILTER_FC_MIN, FILTER_FC_MAX, data2);
            sds->m_filter.f.m_fc = scaley_val;
        }
        break;
    default:
        printf("SOMthing else\n");
    }
}
