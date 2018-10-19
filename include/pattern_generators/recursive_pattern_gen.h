#pragma once

#include <defjams.h>
#include <pattern_generator.h>

typedef struct recursive_pattern_gen
{
    pattern_generator pg;
    float threshold;
    float divisor;
    float multi;
    int midi_note;
} recursive_pattern_gen;

pattern_generator *new_recursive_pattern_gen(void);
void recursive_pattern_generate(void *self, midi_event *midi_pattern);
void recursive_pattern_gen_status(void *self, wchar_t *status_string);
void recursive_pattern_gen_event_notify(void *self, unsigned int event_type);
void recursive_pattern_gen_set_debug(void *self, bool b);

void recursive_pattern_gen_set_thresh(recursive_pattern_gen *rpg, float val);
void recursive_pattern_gen_set_divisor(recursive_pattern_gen *rpg, float val);
void recursive_pattern_gen_set_multi(recursive_pattern_gen *rpg, float val);
void recursive_pattern_gen_set_midi_note(recursive_pattern_gen *rpg,
                                         int midi_note);
