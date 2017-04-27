#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "defjams.h"
#include "mixer.h"
#include "spork.h"
#include "sequencer_utils.h"
#include "utils.h"

extern mixer *mixr;

spork *new_spork()
{
    spork *s = (spork *)calloc(1, sizeof(spork));

    // LFO
    osc_new_settings((oscillator *)&s->m_lfo);
    lfo_set_soundgenerator_interface(&s->m_lfo);

    // THREE OSC
    osc_new_settings(&s->m_osc1.osc);
    qb_set_soundgenerator_interface(&s->m_osc1);
    s->m_osc1.osc.m_waveform = SINE;

    osc_new_settings(&s->m_osc2.osc);
    qb_set_soundgenerator_interface(&s->m_osc2);
    s->m_osc2.osc.m_waveform = SINE;
    s->m_osc2.osc.m_cents = 0.2;

    osc_new_settings(&s->m_osc3.osc);
    qb_set_soundgenerator_interface(&s->m_osc3);
    s->m_osc3.osc.m_waveform = SQUARE;

    filter_moog_init(&s->m_filter);

    lfo_start_oscillator((oscillator *)&s->m_lfo);
    qb_start_oscillator((oscillator *)&s->m_osc1);
    qb_start_oscillator((oscillator *)&s->m_osc2);
    qb_start_oscillator((oscillator *)&s->m_osc3);

    s->sg.gennext = &spork_gennext;
    s->sg.status = &spork_status;
    s->sg.getvol = &spork_getvol;
    s->sg.setvol = &spork_setvol;
    s->sg.type = SPORK_TYPE;

    s->m_reverb = new_reverb();

    s->m_volume = 0.6;

    return s;
}

double spork_gennext(void *sg)
{
    spork *s = (spork *)sg;

    int bitnom = gimme_a_bitwise_int(0, mixr->cur_sample);
    double bittydob = scaleybum(CHAR_MIN, CHAR_MAX, -1.0, 1.0, bitnom);
    //printf("BITNOM: %d %f\n", bitnom, bittydob);

    s->m_lfo.osc.m_osc_fo = bitnom;
    // LFOOOOOO
    osc_update(&s->m_lfo.osc, "SpoooooooOOOrkFO");
    double unused_quad = 0.0;
    double lfo_out = lfo_do_oscillate(&s->m_lfo.osc, &unused_quad);
    //printf("LFO QUAD:%f\n", unused_quad);
    if (bittydob > 0)
        lfo_out = lfo_out * bittydob;

    //// update OSC
    s->m_osc1.osc.m_fo_mod = lfo_out;
    osc_update((oscillator *)&s->m_osc1, "OSC1");
    s->m_osc2.osc.m_fo_mod = lfo_out;
    osc_update((oscillator *)&s->m_osc2, "OSC2");
    s->m_osc3.osc.m_fo_mod = lfo_out;
    osc_update((oscillator *)&s->m_osc3, "OSC3");
    double osc_out = 0.3 * qb_do_oscillate((oscillator *)&s->m_osc1, NULL) +
                     0.3 * qb_do_oscillate((oscillator *)&s->m_osc2, NULL) +
                     0.3 * qb_do_oscillate((oscillator *)&s->m_osc3, NULL);

    // FILTERRZZZ
    moog_update((filter *)&s->m_filter);
    double filter_out = moog_gennext((filter *)&s->m_filter, osc_out);

    double return_val = 0.;
    reverb_process_audio(s->m_reverb, &filter_out, &return_val, 1, 1);

    return_val = effector(&s->sg, return_val);
    return_val = envelopor(&s->sg, return_val);
    return return_val * s->m_volume;
}

void spork_status(void *self, wchar_t *ss)
{
    spork *s = (spork *)self;
    swprintf(ss, MAX_PS_STRING_SZ, WANSI_COLOR_GREEN "[SPORK] Vol: %.2lf\n"
            "  LFO: Freq: %.2f Octave: %.2f Semitones: %.2f Cents: %.2f\n"
            "  OSC1: Freq: %.2f Octave: %.2f Semitones: %.2f Cents: %.2f\n"
            "  OSC2: Freq: %.2f Octave: %.2f Semitones: %.2f Cents: %.2f\n"
            "  OSC3: Freq: %.2f Octave: %.2f Semitones: %.2f Cents: %.2f\n"
            , s->m_volume
            , s->m_lfo.osc.m_osc_fo, s->m_lfo.osc.m_octave, s->m_lfo.osc.m_semitones, s->m_lfo.osc.m_cents
            , s->m_osc1.osc.m_osc_fo, s->m_osc1.osc.m_octave, s->m_osc1.osc.m_semitones, s->m_osc1.osc.m_cents
            , s->m_osc2.osc.m_osc_fo, s->m_osc2.osc.m_octave, s->m_osc2.osc.m_semitones, s->m_osc2.osc.m_cents
            , s->m_osc3.osc.m_osc_fo, s->m_osc3.osc.m_octave, s->m_osc3.osc.m_semitones, s->m_osc3.osc.m_cents
            );
    wcscat(ss, WANSI_COLOR_RESET);
}

double spork_getvol(void *self)
{
    spork *s = (spork *)self;
    return s->m_volume;
}

void spork_setvol(void *self, double v)
{
    spork *s = (spork *)self;
    s->m_volume = v;
}
