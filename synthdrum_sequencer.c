#include "synthdrum_sequencer.h"
#include "sequencer_utils.h"
#include "mixer.h"
#include <stdlib.h>

extern mixer *mixr;

const int OSC1_SUSTAIN_MS = 15;
const int OSC2_SUSTAIN_MS = 150;

synthdrum_sequencer *new_synthdrum_seq(int drumtype)
{
    printf("New Drum Synth!\n");
    synthdrum_sequencer *sds = calloc(1, sizeof(synthdrum_sequencer));
    sds->drumtype = drumtype;
    sds->vol = 0.7;
    sds->started = false;
    for (int i = 0; i < SEQUENCER_PATTERN_LEN; i++) {
        sds->metadata[i].played  = 0.0;
        sds->metadata[i].playing = 0.0;
    }

    osc_new_settings(&sds->m_osc1.osc);
    qb_set_soundgenerator_interface(&sds->m_osc1);
    sds->m_osc1.osc.m_waveform = NOISE;
    sds->osc1_sustain_len_in_samples = SAMPLE_RATE / 1000. * OSC1_SUSTAIN_MS;
    sds->osc1_sustain_counter = 0;

    osc_new_settings(&sds->m_osc2.osc);
    qb_set_soundgenerator_interface(&sds->m_osc2);
    sds->m_osc2.osc.m_waveform = SINE;
    sds->m_osc2.osc.m_osc_fo = 440;
    sds->osc2_sustain_len_in_samples = SAMPLE_RATE / 1000. * OSC2_SUSTAIN_MS;
    sds->osc2_sustain_counter = 0;

    filter_moog_init(&sds->m_filter);

    envelope_generator_init(&sds->m_eg1);
    envelope_generator_init(&sds->m_eg2);

    sds->sg.gennext = &sds_gennext;
    sds->sg.status = &sds_status;
    sds->sg.getvol = &sds_getvol;
    sds->sg.setvol = &sds_setvol;

    return sds;
}

void sds_status(void *self, wchar_t *ss)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer*) self;
    swprintf(ss, MAX_PS_STRING_SZ, WANSI_COLOR_GREEN "[SYNTHDRUM] Type: %s Vol: %.2lf Ptn: %d\n",
             sds->drumtype == KICK ? "drum" : "snare", sds->vol, sds->pattern);

	wchar_t pattern_details[128];
    char spattern[17];
    char_binary_version_of_int(sds->pattern, spattern);
    swprintf(pattern_details, 127, L"\n      [%s]", spattern);
    wcscat(ss, pattern_details);
    wcscat(ss, WANSI_COLOR_RESET);

}

void sds_setvol(void *self, double v)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer*) self;
    sds->vol = v;
    return;
}

double sds_gennext(void *self)
{
    synthdrum_sequencer *sds = (synthdrum_sequencer*) self;
    double val = 0.0;

    int step_seq_idx = mixr->sixteenth_note_tick % SEQUENCER_PATTERN_LEN;

    if (!sds->started) {
        if (step_seq_idx == 0)
            sds->started = true;
        else return val;
    }

    int bit_position = 1 << (15 - step_seq_idx);
    if ((sds->pattern & bit_position) && mixr->is_sixteenth) {
        sds_trigger(sds);
    }

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

    if (mixr->debug_mode)
        if (eg1 > 0.0 || eg2 > 0.0)
            printf("EG1: %f EG2: %f\n", eg1, eg2);

    osc_update(&sds->m_osc1.osc, "NOISEOSC");
    osc_update(&sds->m_osc2.osc, "SINEOSC");

    double osc1 = qb_do_oscillate(&sds->m_osc1.osc, NULL);
    double osc2 = qb_do_oscillate(&sds->m_osc2.osc, NULL);

    if (mixr->debug_mode)
        if (osc1 > 0.0 || osc2 > 0.0)
            printf("oSC1: %f OSC2: %f\n", osc1, osc2);

    val = eg1 * osc1
        + eg2 * osc2;

    return val;
}

double sds_getvol(void *self)
{ 
    synthdrum_sequencer *sds = (synthdrum_sequencer*) self;
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
