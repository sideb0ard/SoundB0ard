#pragma once

#include "defjams.h"
#include "pattern_generator.h"

#define MAX_PARTS 16
#define MAX_ALLOWED_PART_SIZES 16
#define MAX_ALLOWED_PATTERNS 500
#define MAX_ALLOWED_PART_SIZES_AS_STRING 128

enum
{
    INTDIV_STATIC,
    INTDIV_RANDOM,
    INTDIV_NUM_MODES
};

typedef struct intdiv
{
    pattern_generator sg;

    unsigned int mode;
    int parts[MAX_PARTS];
    int num_parts;

    char allowed_part_sizes_as_string[MAX_ALLOWED_PART_SIZES_AS_STRING];
    int allowed_part_sizes[MAX_ALLOWED_PART_SIZES];
    int num_allowed_part_sizes;

    uint16_t patterns[MAX_ALLOWED_PATTERNS];
    int num_patterns;
    int selected_pattern;
} intdiv;

pattern_generator *new_intdiv();
void intdiv_generate(void *self, void *data);
void intdiv_status(void *self, wchar_t *status_string);
void intdiv_event_notify(void *self, broadcast_event event);
void intdiv_set_debug(void *self, bool b);
void intdiv_set_allowed_part_sizes(intdiv *id, char const *allowed_parts);
void intdiv_set_num_parts(intdiv *id, int num_parts);
void intdiv_print_patterns(intdiv *id);
void intdiv_set_selected_pattern(intdiv *id, unsigned int pat_num);
void intdiv_set_mode(intdiv *id, unsigned int mode);
