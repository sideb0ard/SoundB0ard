#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "filter_onepole.h"
#include "filter_csem.h"
#include "fm.h"
#include "table.h"
#include "utils.h"

extern wtable *wave_tables[5];

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

    if (strncmp(osc1, "square", 10) == 0)
        fm->osc1 = new_oscil(osc1_freq, SQUARE);
    else if (strncmp(osc1, "saw_u", 10) == 0)
        fm->osc1 = new_oscil(osc1_freq, SAW_U);
    else if (strncmp(osc1, "saw_d", 10) == 0)
        fm->osc1 = new_oscil(osc1_freq, SAW_D);
    else if (strncmp(osc1, "sine", 10) == 0)
        fm->osc1 = new_oscil(osc1_freq, SINE);
    else // tri
        fm->osc1 = new_oscil(osc1_freq, TRI);

    if (strncmp(osc2, "square", 10) == 0)
        fm->osc2 = new_oscil(osc2_freq, SQUARE);
    else if (strncmp(osc2, "saw_u", 10) == 0)
        fm->osc2 = new_oscil(osc2_freq, SAW_U);
    else if (strncmp(osc2, "saw_d", 10) == 0)
        fm->osc2 = new_oscil(osc2_freq, SAW_D);
    else if (strncmp(osc2, "sine", 10) == 0)
        fm->osc2 = new_oscil(osc2_freq, SINE);
    else // tri
        fm->osc2 = new_oscil(osc2_freq, TRI);

    fm->osc2->m_cents = 2.5; // +2.5 cents detuned

    // lfo
    fm->lfo = new_oscil(DEFAULT_LFO_RATE, SINE);
    oscil_setvol(fm->lfo, 0.0);

    // ENVELOPE GENERATOR
    fm->env = new_envelope_generator();

    // FILTER - VA ONEPOLE
    fm->filter = new_filter_onepole();

    // FILTER - VA CSEM
    //fm->filter = new_filter_csem();

    // Digitally Controlled Amplitude
    fm->dca = new_dca();

    fm->vol = 0.7;
    fm->cur_octave = 2;
    fm->sustain = 0;
    // fm->osc1->sound_generator.setvol(fm->osc1, 0.5);
    // fm->osc2->sound_generator.setvol(fm->osc2, 0.5);
    //
    fm->m_filter_keytrack = true;
    fm->m_filter_keytrack_intensity = 1.0;

    fm->sound_generator.gennext = &fm_gennext;
    fm->sound_generator.status = &fm_status;
    fm->sound_generator.setvol = &fm_setvol;
    fm->sound_generator.getvol = &fm_getvol;
    fm->sound_generator.type = FM_TYPE;

    return fm;
}

void fm_change_osc_wave_form(FM *self, int oscil)
{
    OSCIL *o;
    if (oscil == 0)
        o = self->lfo;
    else if (oscil == 1)
        o = self->osc1;
    else if (oscil == 2)
        o = self->osc2;
    else
        return;

    wave_type type = (o->wav + 1) % 5;
    printf("Changing wav types to %d\n", type);
    o->wav = type;

    switch (type) {
    case SAW_D: {
        osc_set_wave(o, SAW_D);
        break;
    }
    case SAW_U: {
        osc_set_wave(o, SAW_U);
        break;
    }
    case TRI: {
        osc_set_wave(o, TRI);
        break;
    }
    case SQUARE: {
        osc_set_wave(o, SQUARE);
        break;
    }
    case SINE:
    default: {
        osc_set_wave(o, SINE);
        break;
    }
    }
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

void change_octave(void *self, int direction)
{
    FM *fm = (FM *)self;
    int octave = fm->cur_octave;
    if (direction == UP)
        octave++;
    else
        octave--;

    if (octave >= 0 && octave < 6)
        fm->cur_octave = octave;
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
    set_freq(fm->osc1, val);
    set_freq(fm->osc2, val);
}

void keypress_on(FM *self, double freq)
{
    mfm(self, freq);

    osc_start(self->osc1);
    osc_start(self->osc2);
    osc_start(self->lfo);
    start_eg(self->env);
}

void keypress_off(void *self)
{
    (void)self;
    // FM *fm = (FM *)self;
    // osc_stop(fm->osc1);
    // osc_stop(fm->osc2);
    // osc_stop(fm->lfo);
    // stop_eg(fm->env);
}

// void fm_gennext(void* self, double* frame_vals, int framesPerBuffer)
double fm_gennext(void *self)
{
    FM *fm = (FM *)self;

    if (fm->osc1->m_note_on) {

        // ARTICULATION BLOCK
        double lfo_out = fm->lfo->sound_generator.gennext(fm->lfo);
        //printf("LFO out! %f\n", lfo_out);
        double biased_eg = 0.0;
        double eg_out = env_generate(fm->env, &biased_eg);

        // CALC ENV GEN -> OSC MOD
        // TODO : implement a controller for eg1_osc_intensity
        double eg_osc_mod = fm->env->m_eg1_osc_intensity * OSC_FQ_MOD_RANGE * biased_eg;
        //double eg_osc_mod = 1 * OSC_FQ_MOD_RANGE * biased_eg;

        set_fq_mod_exp(fm->osc1, OSC_FQ_MOD_RANGE * lfo_out + eg_osc_mod);
        set_fq_mod_exp(fm->osc2, OSC_FQ_MOD_RANGE * lfo_out + eg_osc_mod);

        osc_update(fm->osc1);
        osc_update(fm->osc2);

         if ( fm->m_filter_keytrack == ON )
            fm->filter->bc_filter->m_fc_control =
                fm->osc1->freq * fm->m_filter_keytrack_intensity;
        
        filter_set_fc_mod(fm->filter->bc_filter, FILTER_FC_MOD_RANGE * eg_out);
        onepole_update(fm->filter);
        //csem_update(fm->filter);

        dca_set_eg_mod(fm->dca, eg_out * 1.0);
        dca_update(fm->dca);

        double osc1_val = fm->osc1->sound_generator.gennext(fm->osc1);
        double osc2_val = fm->osc2->sound_generator.gennext(fm->osc2);

        double osc_out = 0.5 * osc1_val + 0.5 * osc2_val;

        double filter_out = onepole_gennext(fm->filter, osc_out);
        //double filter_out = csem_gennext(fm->filter, osc_out);

        //printf("OSCOUT: %f // FILTEROUT: %f\n", osc_out, filter_out);

        double out_left;
        double out_right;
        dca_gennext(fm->dca, filter_out, filter_out, &out_left, &out_right);

        double dca_out = 0.5 * out_left + 0.5 * out_right;

        if ((get_state(fm->env)) == 0) {
            osc_stop(fm->osc1);
            osc_stop(fm->osc2);
            osc_stop(fm->lfo);
            stop_eg(fm->env);
        }

        // my old schools..>
        dca_out = effector(&fm->sound_generator, dca_out);
        dca_out = envelopor(&fm->sound_generator, dca_out);

        return fm->vol * dca_out;
    }
    else {
        return 0.0;
    }
}

void fm_set_sustain(FM *self, int val)
{
    self->sustain = val;
    printf("Set sustain to %d\n", val);
}

void fm_add_melody_loop(void *self, melody_loop *mloop)
{
    FM *fm = (FM *) self;
    if ( fm->melody_loop_num < 10 )
        fm->mloops[fm->melody_loop_num++] = mloop;
}
