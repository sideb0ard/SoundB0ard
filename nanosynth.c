#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "defjams.h"
#include "filter_ckthreefive.h"
#include "filter_csem.h"
#include "filter_onepole.h"
#include "nanosynth.h"
#include "table.h"
#include "utils.h"

extern wtable *wave_tables[5];

nanosynth *new_nanosynth()
{
    nanosynth *ns;
    ns = (nanosynth *)calloc(1, sizeof(nanosynth));
    if (ns == NULL)
        return NULL;

    ns->osc1 = (oscillator *) qb_osc_new();
    ns->osc2 = (oscillator *) qb_osc_new();
    ns->osc2->m_cents = 2; // +2.5 cents detuned

    ns->lfo = (oscillator *) lfo_new();

    ns->eg1 = new_envelope_generator();

    // FILTER - VA ONEPOLE
    // ns->filter = new_filter_onepole();

    // FILTER - VA CK35
    ns->filter = new_filter_ck35();

    // FILTER - VA CSEM
    // ns->filter = new_filter_csem();

    // Digitally Controlled Amplitude
    ns->dca = new_dca();

    // mod matrix setup
    ns->m_modmatrix = new_modmatrix();

    ns->m_default_mod_intensity = 1.0;
    ns->m_default_mod_range = 1.0;
    ns->m_osc_fq_mod_range = OSC_FO_MOD_RANGE;
    ns->m_filter_mod_range = FILTER_FC_MOD_RANGE;
    ns->m_eg1_dca_intensity = 1.0;
    ns->m_eg1_osc_intensity = 0.0;

    matrixrow *row = NULL;
    // LFO -> ALL OSC FO
    row = create_matrix_row(SOURCE_LFO1, DEST_ALL_OSC_FO,
                            &ns->m_default_mod_intensity,
                            &ns->m_osc_fq_mod_range, TRANSFORM_NONE, true);
    add_matrix_row(ns->m_modmatrix, row);

    // EG1 -> ALL OSC FO
    row = create_matrix_row(SOURCE_BIASED_EG1, DEST_ALL_OSC_FO,
                            &ns->m_eg1_osc_intensity, &ns->m_osc_fq_mod_range,
                            TRANSFORM_NONE, true);
    add_matrix_row(ns->m_modmatrix, row);

    // EG1 -> FILTER1 FC
    row = create_matrix_row(SOURCE_BIASED_EG1, DEST_ALL_FILTER_FC,
                            &ns->m_default_mod_intensity,
                            &ns->m_filter_mod_range, TRANSFORM_NONE, true);
    add_matrix_row(ns->m_modmatrix, row);

    // EG1 -> DCA EG
    row = create_matrix_row(SOURCE_EG1, DEST_DCA_EG, &ns->m_eg1_dca_intensity,
                            &ns->m_default_mod_range, TRANSFORM_NONE, true);
    add_matrix_row(ns->m_modmatrix, row);

    // NOTE NUMBER -> FILTER FC CONTROL
    row = create_matrix_row(SOURCE_MIDI_NOTE_NUM, DEST_ALL_FILTER_KEYTRACK,
                            &ns->m_filter_keytrack_intensity,
                            &ns->m_default_mod_range,
                            TRANSFORM_NOTE_NUMBER_TO_FREQUENCY, true);
    add_matrix_row(ns->m_modmatrix, row);

    // end mod matrix setup ///////////////////////////////////
    //
    // mod matrix routings ////////////////////////////////////

    ns->osc1->g_modmatrix = ns->m_modmatrix;
    ns->osc1->m_mod_source_fo = DEST_OSC1_FO;
    ns->osc1->m_mod_source_amp = DEST_OSC1_OUTPUT_AMP;

    ns->osc2->g_modmatrix = ns->m_modmatrix;
    ns->osc2->m_mod_source_fo = DEST_OSC2_FO;
    ns->osc2->m_mod_source_amp = DEST_OSC2_OUTPUT_AMP;

    ns->filter->bc_filter->global_modmatrix = ns->m_modmatrix;
    ns->filter->bc_filter->m_mod_source_fc = DEST_FILTER1_FC;
    ns->filter->bc_filter->m_mod_source_fc_control = DEST_ALL_FILTER_KEYTRACK;

    // modulators - they write their outputs into
    // what will be a Source for something else

    ns->lfo->g_modmatrix = ns->m_modmatrix;
    ns->lfo->m_mod_dest_output1 = SOURCE_LFO1;
    ns->lfo->m_mod_dest_output2 = SOURCE_LFO1Q;

    ns->eg1->global_modmatrix = ns->m_modmatrix;
    ns->eg1->m_mod_dest_eg_output = SOURCE_EG1;
    ns->eg1->m_mod_dest_eg_biased_output = SOURCE_BIASED_EG1;

    ns->dca->global_modmatrix = ns->m_modmatrix;
    ns->dca->m_mod_source_eg = DEST_DCA_EG;
    ns->dca->m_mod_source_amp_db = DEST_NONE;
    ns->dca->m_mod_source_velocity = DEST_NONE;
    ns->dca->m_mod_source_pan = DEST_NONE;

    ns->vol = 0.7;
    ns->cur_octave = 2;
    ns->sustain = 0;

    ns->m_filter_keytrack = true;
    ns->m_filter_keytrack_intensity = 1.0;

    ns->sound_generator.gennext = &nanosynth_gennext;
    ns->sound_generator.status = &nanosynth_status;
    ns->sound_generator.setvol = &nanosynth_setvol;
    ns->sound_generator.getvol = &nanosynth_getvol;
    ns->sound_generator.type = NANOSYNTH_TYPE;

    return ns;
}

//void nanosynth_change_osc_wave_form(nanosynth *self, int oscil)
//{
//    OSCIL *o;
//    if (oscil == 0)
//        o = self->lfo;
//    else if (oscil == 1)
//        o = self->osc1;
//    else if (oscil == 2)
//        o = self->osc2;
//    else
//        return;
//
//    wave_type type = (o->wav + 1) % 5;
//    printf("Changing wav types to %d\n", type);
//    o->wav = type;
//
//    switch (type) {
//    case SAW_D: {
//        osc_set_wave(o, SAW_D);
//        break;
//    }
//    case SAW_U: {
//        osc_set_wave(o, SAW_U);
//        break;
//    }
//    case TRI: {
//        osc_set_wave(o, TRI);
//        break;
//    }
//    case SQUARE: {
//        osc_set_wave(o, SQUARE);
//        break;
//    }
//    case SINE:
//    default: {
//        osc_set_wave(o, SINE);
//        break;
//    }
//    }
//}

void nanosynth_setvol(void *self, double v)
{
    nanosynth *ns = (nanosynth *)self;
    if (v < 0.0 || v > 1.0) {
        return;
    }
    ns->vol = v;
}

double nanosynth_getvol(void *self)
{
    nanosynth *ns = (nanosynth *)self;
    return ns->vol;
}

void change_octave(void *self, int direction)
{
    nanosynth *ns= (nanosynth *)self;
    int octave = ns->cur_octave;
    if (direction == UP)
        octave++;
    else
        octave--;

    if (octave >= 0 && octave < 6)
        ns->cur_octave = octave;
}

void nanosynth_status(void *self, char *status_string)
{
    nanosynth *ns = (nanosynth *)self;
    snprintf(status_string, 119,
             ANSI_COLOR_RED "nanosynth! %.2f(freq) "
                            "vol: %.2f" ANSI_COLOR_RESET,
             ns->osc1->m_fo,  ns->vol);
}

void note_on(nanosynth *self, double freq)
{
    self->osc1->m_osc_fo = freq;
    self->osc2->m_osc_fo = freq;

    osc_update(self->osc1);
    osc_update(self->osc2);

    qb_start_oscillator(self->osc1);
    qb_start_oscillator(self->osc2);
    lfo_start_oscillator(self->lfo);
    start_eg(self->eg1);
}

void note_off(void *self)
{
    (void)self;
    // nanosynth *nanosynth = (nanosynth *)self;
    // osc_stop(nanosynth->osc1);
    // osc_stop(nanosynth->osc2);
    // osc_stop(nanosynth->lfo);
    // stop_eg(nanosynth->eg1);
}

// void nanosynth_gennext(void* self, double* frame_vals, int framesPerBuffer)
double nanosynth_gennext(void *self)
{
    nanosynth *ns = (nanosynth *)self;

    if (ns->osc1->m_note_on) {
        //printf("OSX1 note on!\n");

        //// NEW SHIT - moD MATRiX stYle /////////////////

        do_modulation_matrix(ns->m_modmatrix, 0);

        // layer one modulators
        eg_update(ns->eg1);
        //oscil_update(nanosynth->lfo);
        osc_update(ns->lfo);

        double biased_eg = 0.0;
        eg_generate(ns->eg1, &biased_eg);
        //nanosynth->lfo->sound_generator.gennext(nanosynth->lfo);
        lfo_do_oscillate(ns->lfo, NULL);

        do_modulation_matrix(ns->m_modmatrix, 1);

        dca_update(ns->dca);
        ck_update(ns->filter);

        osc_update(ns->osc1);
        osc_update(ns->osc2);

        //double osc1_val = nanosynth->osc1->sound_generator.gennext(nanosynth->osc1);
        //double osc2_val = nanosynth->osc2->sound_generator.gennext(nanosynth->osc2);
        double osc1_val = qb_do_oscillate(ns->osc1, NULL);
        double osc2_val = qb_do_oscillate(ns->osc2, NULL);

        double osc_out = 0.5 * osc1_val + 0.5 * osc2_val;

        // double filter_out = onepole_gennext(nanosynth->filter, osc_out);
        double filter_out = ck_gennext(ns->filter, osc_out);
        // double filter_out = csem_gennext(nanosynth->filter, osc_out);

        // printf("OSCOUT: %f // FILTEROUT: %f\n", osc_out, filter_out);

        double out_left;
        double out_right;
        dca_gennext(ns->dca, filter_out, filter_out, &out_left, &out_right);

        double dca_out = 0.5 * out_left + 0.5 * out_right;

        if ((get_state(ns->eg1)) == 0) {
            qb_stop_oscillator(ns->osc1);
            qb_stop_oscillator(ns->osc2);
            lfo_stop_oscillator(ns->lfo);
            stop_eg(ns->eg1);
        }

        //// my old schools..>
        //dca_out = effector(&nanosynth->sound_generator, dca_out);
        //dca_out = envelopor(&nanosynth->sound_generator, dca_out);

        return ns->vol * dca_out;
        //return 0.0;
    }
    else {
        return 0.0;
    }
}

void nanosynth_set_sustain(nanosynth *self, int val)
{
    self->sustain = val;
    printf("Set sustain to %d\n", val);
}

void nanosynth_add_melody_loop(void *self, melody_loop *mloop)
{
    nanosynth *ns = (nanosynth *)self;
    if (ns->melody_loop_num < 10)
        ns->mloops[ns->melody_loop_num++] = mloop;
}
