#ifndef ENVELOPE_H
#define ENVELOPE_H

#include "envelope_generator.h"
#include "fx.h"

enum
{
    ENV_MODE_TRIGGER,
    ENV_MODE_SUSTAIN
};

typedef struct envelope
{
    fx m_fx;
    envelope_generator eg;
    bool started;

    unsigned int env_mode; // trigger or sustain
    double env_length_bars;
    int env_length_ticks;
    int env_length_ticks_counter;
    int release_tick;

    unsigned int eg_state;
    bool debug;
} envelope;

envelope *new_envelope(void);
void envelope_reset(envelope *e);

void envelope_status(void *self, char *string);
stereo_val envelope_process_audio(void *self, stereo_val input);

void envelope_calculate_timings(envelope *e);
void envelope_event_notify(void *self, broadcast_event event);

void envelope_set_length_bars(envelope *e, double length_bars);
void envelope_set_attack_ms(envelope *e, double val);
void envelope_set_decay_ms(envelope *e, double val);
void envelope_set_sustain_lvl(envelope *e, double val);
void envelope_set_release_ms(envelope *e, double val);
void envelope_set_type(envelope *e, unsigned int type); // analog or digital
void envelope_set_mode(envelope *e, unsigned int mode); // sustain or trigger
void envelope_set_debug(envelope *e, bool b);

#endif
