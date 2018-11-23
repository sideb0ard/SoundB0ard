#pragma once

#include "defjams.h"
#include "pattern_generator.h"

enum
{
    COMPLEX,
    NUM_JUGGLER_STYLES
};

typedef struct juggler
{
    pattern_generator sg;
    unsigned int juggler_style;
} juggler;

pattern_generator *new_juggler(unsigned int type);
void juggler_generate(void *self, void *data);
void juggler_status(void *self, wchar_t *status_string);
void juggler_event_notify(void *self, unsigned int event_type);
void juggler_set_debug(void *self, bool b);
void juggler_set_style(juggler *m, unsigned int type);
