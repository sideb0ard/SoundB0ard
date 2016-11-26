#ifndef MIXER_H
#define MIXER_H

#include <portaudio.h>

#include "drumr.h"
#include "effect.h"
#include "nanosynth.h"
#include "sbmsg.h"
#include "sound_generator.h"
#include "table.h"

#define INITIAL_SIGNAL_SIZE 4

typedef struct t_mixer {

    SOUNDGEN **sound_generators;
    int soundgen_num;  // actual number of SGs
    int soundgen_size; // number of memory slots reserved for SGszz

    int delay_on;

    EFFECT **effects;
    int effects_num;
    int effects_size;

    unsigned midi_control_destination;
    int active_midi_soundgen_num;
    int active_midi_soundgen_effect_num;

    int bpm;
    int samples_per_midi_tick;
    int sixteenth_note_tick;
    int cur_sample; // inverse of SAMPLE RATE
    int tick;       //

    double volume;
    int keyboard_octave;

} mixer;

typedef struct {
    mixer *mixr;
    float delay[(int)SAMPLE_RATE / 8];
    int delay_p;
    float left_phase;
    float right_phase;
} paData;

mixer *new_mixer(void);

void mixer_ps(mixer *mixr);
void mixer_update_bpm(mixer *mixr, int bpm);
int add_bitwize(mixer *mixr, int pattern);
int add_osc(mixer *mixr, double freq, wave_type w);
int add_nanosynth(mixer *mixr);
int add_drum(mixer *mixr, char *filename, char *pattern);
int add_sampler(mixer *mixr, char *filename, double loop_len);
int add_sound_generator(mixer *mixr, SBMSG *sbm);
int add_effect(mixer *mixr);
void mixer_vol_change(mixer *mixr, float vol);
void vol_change(mixer *mixr, int sig, float vol);
void freq_change(mixer *mixr, int sig, float freq);
void delay_toggle(mixer *mixr);

double gen_next(mixer *mixr);
// void gen_next(mixer* mixr, int framesPerBuffer, float* out);

#endif // MIXER_H
