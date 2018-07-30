#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "envelope.h"
#include "mixer.h"

extern mixer *mixr;

const char *s_eg_state[] = {"OFF",     "ATTACK",  "DECAY",
                            "SUSTAIN", "RELEASE", "SHUTDOWN"};

const char *s_eg_type[] = {"ANALOG", "DIGITAL"};
const char *s_eg_mode[] = {"TRIGGER", "SUSTAIN"};

envelope *new_envelope(void)
{
    envelope *e = calloc(1, sizeof(envelope));
    envelope_reset(e);

    e->m_fx.type = ENVELOPE;
    e->m_fx.enabled = true;
    e->m_fx.status = &envelope_status;
    e->m_fx.process = &envelope_process_audio;
    e->m_fx.event_notify = &envelope_event_notify;

    return e;
}

void envelope_reset(envelope *e)
{
    envelope_generator_init(&e->eg);
    e->eg.m_state = SUSTAIN;
    eg_set_attack_time_msec(&e->eg, 300);
    eg_set_decay_time_msec(&e->eg, 300);
    eg_set_release_time_msec(&e->eg, 300);
    envelope_set_length_bars(e, 1);
}

void envelope_status(void *self, char *status_string)
{
    envelope *e = (envelope *)self;
    snprintf(status_string, MAX_STATIC_STRING_SZ,
             "ENV state:%s len_bars:%.2f len_ticks:%d type:%s mode:%s\n"
             " attack:%.2f decay:%.2f sustain:%.2f release:%.2f // "
             "release_tick:%d",
             s_eg_state[e->eg.m_state], e->env_length_bars, e->env_length_ticks,
             s_eg_type[e->eg.m_eg_mode], s_eg_mode[e->env_mode],
             e->eg.m_attack_time_msec, e->eg.m_decay_time_msec,
             e->eg.m_sustain_level, e->eg.m_release_time_msec, e->release_tick);
}

double envelope_process_audio(void *self, double input)
{
    envelope *e = (envelope *)self;
    double env_out = eg_do_envelope(&e->eg, NULL);
    return input * env_out;
}

void envelope_set_length_bars(envelope *e, double length_bars)
{
    if (length_bars > 0)
    {
        e->env_length_bars = length_bars;
        envelope_calculate_timings(e);
    }
}

void envelope_calculate_timings(envelope *e)
{
    mixer_timing_info info = mixr->timing_info;
    int release_time_ticks = e->eg.m_release_time_msec * info.ms_per_midi_tick;

    e->env_length_ticks = info.loop_len_in_ticks * e->env_length_bars;
    int release_tick = e->env_length_ticks - release_time_ticks;
    if (release_tick > 0)
        e->release_tick = release_tick;
    else
    {
        printf("Barfed on yer envelope -- not enuff runway.\n");
    }

    e->env_length_ticks_counter = info.midi_tick % e->env_length_ticks;
}

void envelope_event_notify(void *self, unsigned int event_type)
{
    envelope *e = (envelope *)self;
    switch (event_type)
    {
    case (TIME_BPM_CHANGE):
        envelope_calculate_timings(e);
        break;
    case (TIME_MIDI_TICK):

        // Envelope Length Cycle Bookkeeping
        (e->env_length_ticks_counter)++;
        if (e->env_length_ticks_counter >= e->env_length_ticks)
        {
            eg_start_eg(&e->eg);
            e->env_length_ticks_counter = 0;
        }
        else if (e->eg.m_state == SUSTAIN &&
                 e->env_length_ticks_counter >= e->release_tick)
        {
            eg_note_off(&e->eg);
        }


        break;
    }
}

void envelope_set_type(envelope *e, unsigned int type)
{
    eg_set_eg_mode(&e->eg, type);
}

void envelope_set_mode(envelope *e, unsigned int mode)
{
    if (mode < 2)
        e->env_mode = mode;
}

void envelope_set_attack_ms(envelope *e, double val)
{
    eg_set_attack_time_msec(&e->eg, val);
    envelope_calculate_timings(e);
}

void envelope_set_decay_ms(envelope *e, double val)
{
    eg_set_decay_time_msec(&e->eg, val);
    envelope_calculate_timings(e);
}

void envelope_set_sustain_lvl(envelope *e, double val)
{
    eg_set_sustain_level(&e->eg, val);
    envelope_calculate_timings(e);
}

void envelope_set_release_ms(envelope *e, double val)
{
    eg_set_release_time_msec(&e->eg, val);
    envelope_calculate_timings(e);
}
