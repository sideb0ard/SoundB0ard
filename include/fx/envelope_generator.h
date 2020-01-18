#pragma once

#include <stdbool.h>

#include "modmatrix.h"
#include "synthfunctions.h"

#define EG_MINTIME_MS 1 // these two used for attacjtime, decay and release
#define EG_MAXTIME_MS 5000
#define EG_DEFAULT_STATE_TIME 100

enum
{
    ANALOG,
    DIGITAL
};

enum
{
    OFFF, // name clash in defjams
    ATTACK,
    DECAY,
    SUSTAIN,
    RELEASE,
    SHUTDOWN
};

typedef struct envelope_generator
{

    modmatrix *m_v_modmatrix;

    unsigned m_mod_source_eg_attack_scaling;
    unsigned m_mod_source_eg_decay_scaling;
    unsigned m_mod_source_sustain_override;

    unsigned m_mod_dest_eg_output;
    unsigned m_mod_dest_eg_biased_output;

    global_eg_params *m_global_eg_params;

    /////////////////////////////////////////

    bool drum_mode; // no sustain
    bool m_sustain_override;
    bool m_release_pending;

    unsigned m_eg_mode; // enum above, analog or digital
    // special modes
    bool m_reset_to_zero;
    bool m_legato_mode;
    bool m_output_eg; // i.e. this instance is going direct to output, rather
                      // than into an intermediatery

    // double m_eg1_osc_intensity;
    double m_envelope_output;

    double m_attack_coeff;
    double m_attack_offset;
    double m_attack_tco;

    double m_decay_coeff;
    double m_decay_offset;
    double m_decay_tco;

    double m_release_coeff;
    double m_release_offset;
    double m_release_tco;

    double m_attack_time_msec;
    double m_decay_time_msec;
    double m_release_time_msec;

    double m_shutdown_time_msec;

    double m_sustain_level;

    double m_attack_time_scalar; // for velocity -> attack time mod
    double m_decay_time_scalar;  // for note# -> decay time mod

    double m_inc_shutdown;

    // enum above
    unsigned int m_state;

} envelope_generator;

envelope_generator *new_envelope_generator(void);

void envelope_generator_init(envelope_generator *eg);

unsigned int eg_get_state(envelope_generator *self);
bool eg_is_active(envelope_generator *self);

bool eg_can_note_off(envelope_generator *self);

void eg_reset(envelope_generator *self);

void eg_set_eg_mode(envelope_generator *self, unsigned int mode);

void eg_calculate_attack_time(envelope_generator *self);
void eg_calculate_decay_time(envelope_generator *self);
void eg_calculate_release_time(envelope_generator *self);

void eg_note_off(envelope_generator *self);

void eg_shutdown(envelope_generator *self);

void eg_set_state(envelope_generator *self, unsigned int state);

void eg_set_attack_time_msec(envelope_generator *self, double time);
void eg_set_decay_time_msec(envelope_generator *self, double time);
void eg_set_release_time_msec(envelope_generator *self, double time);
void eg_set_shutdown_time_msec(envelope_generator *self, double time_msec);

void eg_set_sustain_level(envelope_generator *self, double level);
void eg_set_sustain_override(envelope_generator *self, bool b);

void eg_start_eg(envelope_generator *self);
void eg_stop_eg(envelope_generator *self);

void eg_init_global_parameters(envelope_generator *self,
                               global_eg_params *params);

void eg_update(envelope_generator *self);
double eg_do_envelope(envelope_generator *self, double *p_biased_output);

void eg_release(envelope_generator *self);
void eg_set_drum_mode(envelope_generator *self, bool b);
