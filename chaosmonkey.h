#pragma once

#include <stdbool.h>
#include <wchar.h>

#include "sound_generator.h"

typedef struct chaosmonkey {
    SOUNDGEN sound_generator;
    int frequency_of_wakeup;    // seconds
    int chance_of_interruption; // percent likelihood
    bool make_suggestion;
    bool take_action;
} chaosmonkey;

chaosmonkey *new_chaosmonkey(void);

void chaosmonkey_change_wakeup_freq(chaosmonkey *cm, int num_seconds);
void chaosmonkey_change_chance_interrupt(chaosmonkey *cm, int percent);
void chaosmonkey_suggest_mode(chaosmonkey *cm, bool);
void chaosmonkey_action_mode(chaosmonkey *cm, bool);

double chaosmonkey_gen_next(void *self);
void chaosmonkey_status(void *self, wchar_t *ss);
void chaosmonkey_setvol(void *self, double v);
