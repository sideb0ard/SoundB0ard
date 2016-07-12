#pragma once

#include <stdbool.h>

#define EG_MINTIME_MS 50 // these two used for attacjtime, decay and release
#define EG_MAXTIME_MS 5000
#define EG_DEFAULT_STATE 1000
#define EG1_DEFAULT_OSC_INTENSITY 0
#define EG_MIN_OSC_INTENSITY 0
#define EG_MAX_OSC_INTENSITY 0 // TODO - check this

typedef enum { ANALOG, DIGITAL } eg_mode;

typedef enum {
    OFFF, // name clash in defjams
    ATTACK,
    DECAY,
    SUSTAIN,
    RELEASE,
    SHUTDOWN
} state;

typedef struct envelope_generator {
    bool m_reset_to_zero;
    bool m_legato_mode;
    bool m_output_eg; // i.e. this instance is going direct to output, rather
                      // than into an intermediatery

    eg_mode m_eg_mode;

    double m_eg1_osc_intensity;
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
    double m_inc_shutdown;

    state m_state;

} envelope_generator;

envelope_generator *new_envelope_generator(void);

state get_state(envelope_generator *self);
bool is_active(envelope_generator *self);
bool can_note_off(envelope_generator *self);

void reset(envelope_generator *self);
void set_eg_mode(envelope_generator *self, eg_mode mode);

void calculate_attack_time(envelope_generator *self);
void calculate_decay_time(envelope_generator *self);
void calculate_release_time(envelope_generator *self);

void set_attack_time_msec(envelope_generator *self, double time);
void set_decay_time_msec(envelope_generator *self, double time);
void set_release_time_msec(envelope_generator *self, double time);

void note_off(envelope_generator *self);

void set_sustain_level(envelope_generator *self, double level);
void set_sample_rate(envelope_generator *self, double samplerate);

void start_eg(envelope_generator *self);
void stop_eg(envelope_generator *self);
double env_generate(envelope_generator *self, double *p_biased_output);

void eg_release(envelope_generator *self);
