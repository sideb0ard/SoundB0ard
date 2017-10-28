#pragma once

#include <stdbool.h>
#include <wchar.h>

#include "lfo.h"
#include "minisynth.h"
#include "sound_generator.h"

enum
{
    CHAOS_ONE,
    CHAOS_TWO
};

typedef struct chaosmonkey
{
    soundgenerator sound_generator;
    int frequency_of_wakeup;    // seconds
    int chance_of_interruption; // percent likelihood
    bool make_suggestion;
    bool take_action;
    int last_midi_tick;
    int last_sixteenth;
    int soundgen;
    unsigned soundgen_type;
    unsigned chaos_mode;
    lfo m_lfo;
} chaosmonkey;

chaosmonkey *new_chaosmonkey(int soundgen);

void chaosmonkey_change_wakeup_freq(chaosmonkey *cm, int num_seconds);
void chaosmonkey_change_chance_interrupt(chaosmonkey *cm, int percent);
void chaosmonkey_suggest_mode(chaosmonkey *cm, bool);
void chaosmonkey_action_mode(chaosmonkey *cm, bool);

void chaosmonkey_add_note_at_random_time(minisynth *ms, int note);

stereo_val chaosmonkey_gen_next(void *self, mixer_timing_info timing_info);
void chaosmonkey_status(void *self, wchar_t *ss);
void chaosmonkey_setvol(void *self, double v);
