#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "fm.h"
#include "table.h"
#include "lfo.h"
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

    // lfo
    fm->lfo = new_lfo(4, SQUARE);

    // ENVELOPE GENERATOR
    fm->env = new_envelope_generator();

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

// void fm_gennext(void* self, double* frame_vals, int framesPerBuffer)
double fm_gennext(void *self)
{
    FM *fm = (FM *)self;


    double lfo_out = lfo_gennext(fm->lfo);

    //double osc1_val = fm->osc1->sound_generator.gennext(fm->osc1) * pitch_shift_multiplier(lfo_out);
    //double osc2_val = fm->osc2->sound_generator.gennext(fm->osc2) * pitch_shift_multiplier(lfo_out);

    double osc1_val = fm->osc1->sound_generator.gennext(fm->osc1);
    double osc2_val = fm->osc2->sound_generator.gennext(fm->osc2);

    double val = 0.5*osc1_val + 0.5*osc2_val;

    val = effector(&fm->sound_generator, val);
    val = envelopor(&fm->sound_generator, val);

    val = val * generate(fm->env, 0);

    return fm->vol * val;
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
    start_eg(fm->env);
}

void keypress_off(void *self)
{
    FM *fm = (FM *)self;
    stop_eg(fm->env);
}
