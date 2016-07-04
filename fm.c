#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "filter_onepole.h"
#include "fm.h"
#include "table.h"
#include "utils.h"

extern GTABLE *sine_table;
extern GTABLE *square_table;
extern GTABLE *tri_table;
extern GTABLE *saw_up_table;
extern GTABLE *saw_down_table;

FM *new_fm(double freq1, double freq2)
{
    // quick default version
    return new_fm_x("sine", freq1, "sine", freq2);
}

FM *new_fm_x(char *osc1, double osc1_freq, char *osc2, double osc2_freq)
{
    FM *fm;
    fm = (FM *)calloc(1, sizeof(FM));
    if (fm == NULL)
        return NULL;

    // modulator
    if (strncmp(osc1, "square", 10) == 0)
        fm->osc1 = new_oscil(osc1_freq, square_table);
    else if (strncmp(osc1, "saw_u", 10) == 0)
        fm->osc1 = new_oscil(osc1_freq, saw_up_table);
    else if (strncmp(osc1, "saw_d", 10) == 0)
        fm->osc1 = new_oscil(osc1_freq, saw_down_table);
    else if (strncmp(osc1, "sine", 10) == 0)
        fm->osc1 = new_oscil(osc1_freq, sine_table);
    else // tri
        fm->osc1 = new_oscil(osc1_freq, tri_table);

    // carrier
    if (strncmp(osc2, "square", 10) == 0)
        fm->osc2 = new_oscil(osc2_freq, square_table);
    else if (strncmp(osc2, "saw_u", 10) == 0)
        fm->osc2 = new_oscil(osc2_freq, saw_up_table);
    else if (strncmp(osc2, "saw_d", 10) == 0)
        fm->osc2 = new_oscil(osc2_freq, saw_down_table);
    else if (strncmp(osc2, "sine", 10) == 0)
        fm->osc2 = new_oscil(osc2_freq, sine_table);
    else // tri
        fm->osc2 = new_oscil(osc2_freq, tri_table);

    fm->osc2->m_cents = 2.5; // +2.5 cents detuned

    // lfo
    fm->lfo = new_oscil(4, square_table);

    // ENVELOPE GENERATOR
    fm->env = new_envelope_generator();

    // FILTER - VA ONEPOLE
    fm->filter = new_filter_onepole();

    // Digitally Controlled Amplitude
    fm->dca = new_dca();

    fm->vol = 0.0;
    fm->osc1->sound_generator.setvol(fm->osc1, 0.5);
    fm->osc2->sound_generator.setvol(fm->osc2, 0.5);

    fm->sound_generator.gennext = &fm_gennext;
    fm->sound_generator.status = &fm_status;
    fm->sound_generator.setvol = &fm_setvol;
    fm->sound_generator.getvol = &fm_getvol;
    fm->sound_generator.type = FM_TYPE;

    return fm;
}

void fm_setvol(void *self, double v)
{
    FM *fm = (FM *)self;
    if (v < 0.0 || v > 1.0) {
        return;
    }
    fm->vol = v;
}

double fm_getvol(void *self)
{
    FM *fm = (FM *)self;
    return fm->vol;
}

void fm_status(void *self, char *status_string)
{
    FM *fm = (FM *)self;
    snprintf(status_string, 119,
             ANSI_COLOR_RED "FM! modulator: %.2f(freq) %.2f(phase) // carrier: "
                            "%.2f %.2f // vol: %.2f" ANSI_COLOR_RESET,
             fm->osc1->freq, fm->osc1->curphase, fm->osc2->freq,
             fm->osc2->curphase, fm->vol);
}

void mfm(void *self, double val)
{
    FM *fm = (FM *)self;
    freqfunc(fm->osc1, val);
    freqfunc(fm->osc2, val);
}

void keypress_on(void *self)
{
    FM *fm = (FM *)self;
    fm->note_on = true;
    start_eg(fm->env);
}

void keypress_off(void *self)
{
    FM *fm = (FM *)self;
    fm->note_on = false;
    stop_eg(fm->env);
}

// void fm_gennext(void* self, double* frame_vals, int framesPerBuffer)
double fm_gennext(void *self)
{
    FM *fm = (FM *)self;

    if (fm->note_on) {

        // ARTICULATION BLOCK
        double lfo_out = fm->lfo->sound_generator.gennext(fm->lfo);
        double biased_eg = 0.0;
        double eg_out = env_generate(fm->env, &biased_eg);

        // CALC ENV GEN -> OSC MOD
        double eg_osc_mod = 1 * OSC_FQ_MOD_RANGE * biased_eg;

        set_fq_mod_exp(fm->osc1, OSC_FQ_MOD_RANGE * lfo_out + eg_osc_mod);
        set_fq_mod_exp(fm->osc2, OSC_FQ_MOD_RANGE * lfo_out + eg_osc_mod);

        // TODO implement:
        // if ( self->m_filter_key_track == ON )
        //    self->m_filter
        //
        filter_set_fc_mod(fm->filter->bc_filter, FILTER_FC_MOD_RANGE * eg_out);
        onepole_update(fm->filter);

        dca_set_eg_mod(fm->dca, eg_out * 1.0);
        dca_update(fm->dca);

        double osc1_val = fm->osc1->sound_generator.gennext(fm->osc1);
        double osc2_val = fm->osc2->sound_generator.gennext(fm->osc2);


        double osc_out = 0.5 * osc1_val + 0.5 * osc2_val;
        // printf("VAL %f\n", val);
        //
        double filter_out = onepole_gennext(fm->filter, osc_out);
        //printf("FILTER VAL %f\n", filter_out);

        double out_left;
        double out_right;
        dca_gennext(fm->dca, filter_out, filter_out, &out_left, &out_right);
        //dca_gennext(fm->dca, osc_out, osc_out, &out_left, &out_right);

        double dca_out = (out_left + out_right) / 2;

        // val = effector(&fm->sound_generator, val);
        // val = envelopor(&fm->sound_generator, val);

        return fm->vol * dca_out;
    }
    else {
        return 0.0;
    }
}
