#pragma once

#include <stdbool.h>

typedef enum {
    ANALOG,
    DIGITAL
} eg_mode;

typedef enum {
    OFF,
    ATTACK,
    DECAY,
    SUSTAIN,
    RELEASE,
    SHUTDOWN
} state;

typedef struct envelope_generator {
    bool        m_b_reset_to_zero;
    bool        m_b_legato_mode;
    bool        m_b_output_eg;

    eg_mode     m_u_eg_mode;
    eg_mode     m_eg_mode;

    state       m_u_state;
    state       m_state;

    double      m_d_samplerate;
    double      m_d_envelope_output;

    double      m_d_attack_coeff;
    double      m_d_attack_offset;
    double      m_d_attack_tco;

    double      m_d_decay_coeff;
    double      m_d_decay_offset;
    double      m_d_decay_tco;

    double      m_d_release_coeff;
    double      m_d_release_offset;
    double      m_d_release_tco;

    double      m_d_attack_time_msec;
    double      m_d_decay_time_msec;
    double      m_d_release_time_msec;
    double      m_d_release_time_msec;
    double      m_d_shutdown_time_msec;

    double      m_d_sustain_level;
    double      m_d_inc_shutdown;

} envelope_generator;

envelope_generator *new_envelope_generator();

state   get_state(envelope_generator *self);
bool    is_active(envelope_generator *self);
bool    can_note_off(envelope_generator *self);

void    reset(envelope_generator *self);
void    set_eg_mode(envelope_generator *self, eg_mode mode);

void    calculate_attack_time(envelope_generator *self);
void    calculate_decay_time(envelope_generator *self);
void    calculate_release_time(envelope_generator *self);

void    set_attack_time_msec(envelope_generator *self, double time);
void    set_decay_time_msec(envelope_generator *self, double time);
void    set_release_time_msec(envelope_generator *self, double time);

void    set_sustain_level(envelope_generator *self, double level);
void    set_sample_rate(envelope_generator *self, double samplerate);

void    start_eg(envelope_generator *self);
void    stop_eg(envelope_generator *self);
double  do_envelope(envelope_generator *self, double *p_biased_output);



