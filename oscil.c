#include <stdlib.h>

#include "defjams.h"
#include "mixer.h"
#include "oscil.h"
#include "sound_generator.h"
#include "table.h"
#include "utils.h"

extern mixer *mixr;

OSCIL *new_oscil(double freq, GTABLE *gt)
{
    OSCIL *p_osc;
    p_osc = (OSCIL *)calloc(1, sizeof(OSCIL));
    if (p_osc == NULL)
        return NULL;

    p_osc->freq = freq;
    p_osc->incr = TABRAD * freq;

    p_osc->vol = 0.7; // TODO get rid of vol and use amp
    p_osc->m_amp = 1.0;

    printf("NEW OSCILT! - TABRAD IS %f // freq is %f\n", TABRAD, freq);
    p_osc->gtable = gt;
    p_osc->dtablen = (double)TABLEN;

    p_osc->sound_generator.gennext = &oscil_gennext;
    p_osc->sound_generator.status = &oscil_status;
    p_osc->sound_generator.getvol = &oscil_getvol;
    p_osc->sound_generator.setvol = &oscil_setvol;
    p_osc->sound_generator.type = OSCIL_TYPE;

    // p_osc->voladj = &volfunc;
    p_osc->freqadj = &set_freq;
    p_osc->incradj = &incrfunc;

    p_osc->m_note_on = false;
    p_osc->m_midi_note_number = 0;

    p_osc->m_pulse_width = OSC_PULSEWIDTH_DEFAULT;
    p_osc->m_pw_control = OSC_PULSEWIDTH_DEFAULT;
    p_osc->m_pn_register = rand();

    p_osc->m_rsh_counter = -1;
    p_osc->m_rsh_value = 0.0;
    p_osc->m_amp_mod = 1.0;
    p_osc->m_fq_mod_exp = 0.0;
    p_osc->m_fq_mod_lin = 0.0;
    p_osc->m_phase_mod = 0.0;
    p_osc->m_pitch_bend_mod = 0.0;
    p_osc->m_pw_mod = 0.0;
    p_osc->m_octave = 0.0;
    p_osc->m_semitones = 0.0;
    p_osc->m_cents = 0.0;
    p_osc->m_fq_ratio = 1.0;
    p_osc->m_lfo_mode = 0;

    return p_osc;
}

void incrfunc(OSCIL *p_osc, double v) { p_osc->incr = v; }

double oscil_getvol(void *self)
{
    OSCIL *p_osc = (OSCIL *)self;
    return p_osc->vol;
}

void oscil_setvol(void *self, double v)
{
    OSCIL *p_osc = (OSCIL *)self;
    if (v < 0.0 || v > 1.0) {
        return;
    }
    p_osc->vol = v;
}

void freqinc(OSCIL *p_osc, int direction)
{
    printf("FREQINC!\n");
    int freq;
    if (direction == UP)
        freq = p_osc->freq + 2;
    else
        freq = p_osc->freq - 1;
    set_freq(p_osc, freq);
}

void set_freq(OSCIL *p_osc, double f)
{
    if (f >= OSC_FQ_MIN && f <= OSC_FQ_MAX) {
        p_osc->freq = f;
        p_osc->incr = TABRAD * f;
    }
}

void set_fq_mod_exp(OSCIL *self, double mod) { self->m_fq_mod_exp = mod; }

// void oscil_gennext(void* self, double* frame_vals, int framesPerBuffer) //
// interpolating
//
double oscil_gennext(void *self)
{
    OSCIL *p_osc = (OSCIL *)self;

    // GET BASIC VAL
    int base_index = (int)(p_osc->curphase);
    unsigned long next_index = base_index + 1;
    double frac, slope, val;
    double dtablen = p_osc->dtablen, curphase = p_osc->curphase;
    double *table = p_osc->gtable->table;
    double vol = p_osc->vol;

    frac = curphase - base_index;
    val = table[base_index];
    slope = table[next_index] - val;

    val += (frac * slope);
    curphase += p_osc->incr;

    while (curphase >= dtablen)
        curphase -= dtablen;
    while (curphase < 0.0)
        curphase += dtablen;

    p_osc->curphase = curphase;
    // END BASIC VAL

    val = val * p_osc->m_fq_ratio *
          pitch_shift_multiplier(p_osc->m_fq_mod_exp + p_osc->m_octave * 12.0 +
                                 p_osc->m_semitones + p_osc->m_cents / 100.0);
    val += p_osc->m_fq_mod_lin;

    return val * vol;
}

void oscil_status(void *self, char *status_string)
{
    OSCIL *p_osc = self;
    snprintf(
        status_string, 119, ANSI_COLOR_YELLOW
        "freq: %f vol: %f incr: %f cur: %f num_effects: %d" ANSI_COLOR_RESET,
        p_osc->freq, p_osc->vol, p_osc->incr, p_osc->curphase,
        p_osc->sound_generator.effects_num);
}

void osc_reset(OSCIL *self)
{
    self->m_dpw_square_modulator = -1.0;
    self->m_dpw_z1 = 0.0;
    self->m_pn_register = rand();
    self->m_rsh_counter = -1.0;
    self->m_amp_mod = 1.0;
    self->m_pw_mod = 0.0;
    self->m_pitch_bend_mod = 0.0;
    self->m_fq_mod_exp = 0.0;
    self->m_fq_mod_lin = 0.0;
    self->m_phase_mod = 0.0;
}

void osc_start(OSCIL *self)
{
    osc_reset(self);
    self->m_note_on = true;
}

void osc_stop(OSCIL *self) { self->m_note_on = false; }

void set_midi_note_num(OSCIL *self, int midi_note_num)
{
    self->m_midi_note_number = midi_note_num;
}

// void set_pitch_bend_mod(OSCIL *self, double bend) {

void pitch_bend(OSCIL *self, double cents) { self->m_cents = cents; }
