#pragma once

#include "audiofile_data.h"
#include "oscillator.h"

enum
{
    SAMPLE_LOOP,
    SAMPLE_ONESHOT
};

typedef struct sample_oscillator sampleosc;
typedef struct sample_oscillator
{
    oscillator osc;
    audiofile_data afd;
    int orig_pitch_midi_{36}; // default is C2

    bool is_single_cycle;
    bool is_pitchless;

    unsigned int loop_mode;

    double m_read_idx;

} sample_oscillator;

void sampleosc_init(sampleosc *sosc, std::string filename);

void sampleosc_set_oscillator_interface(sampleosc *self);

void sampleosc_start_oscillator(oscillator *self);
void sampleosc_stop_oscillator(oscillator *self);
void sampleosc_reset_oscillator(oscillator *self);
void sampleosc_update(oscillator *self);

double sampleosc_read_sample_buffer(sampleosc *sosc);
double sampleosc_do_oscillate(oscillator *self, double *quad_phase_output);
