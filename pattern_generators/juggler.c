#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "juggler.h"
#include "mixer.h"
#include "pattern_generator.h"
#include "pattern_utils.h"
#include "utils.h"
#include <euclidean.h>
#include <markov.h>

extern mixer *mixr;
static char *s_juggler_styles[] = {"COMPLEX"};

pattern_generator *new_juggler(unsigned int style)
{
    juggler *m = calloc(1, sizeof(juggler));
    if (!m)
    {
        printf("WOOF!\n");
        return NULL;
    }
    m->juggler_style = style;
    m->max_depth = 3;
    m->pct_probability = 60;

    m->sg.status = &juggler_status;
    m->sg.generate = &juggler_generate;
    m->sg.event_notify = &juggler_event_notify;
    m->sg.set_debug = &juggler_set_debug;
    m->sg.type = JUGGLER;

    return (pattern_generator *)m;
}

void juggler_status(void *self, wchar_t *wstring)
{
    juggler *j = (juggler *)self;
    swprintf(wstring, MAX_STATIC_STRING_SZ,
             L"[" WANSI_COLOR_WHITE "JUGGLER PATTERN GEN ] - " WCOOL_COLOR_PINK
             "debug:%d style:(%d)%s max_depth:%d pct_probability:%d\n",
             j->debug, j->juggler_style, s_juggler_styles[j->juggler_style],
             j->max_depth, j->pct_probability);
}

void _juggler_recursive_generation(juggler *j, int start_idx, int pattern_len,
                                   int depth, midi_event *midi_pattern)
{
    if (depth == j->max_depth)
        return;

    if (j->debug)
    {
        char *spacer = calloc(sizeof(char), depth + 1);
        for (int i = 0; i < depth; i++)
            strcat(spacer, " ");
        printf("%sSTART idx:%d len:%d depth:%d\n", spacer, start_idx,
               pattern_len, depth);
        free(spacer);
    }

    int generator = rand() % 2;
    int rand_steps = rand() % 11;
    uint16_t bit_pattern = 0;
    switch (generator)
    {
    case (0):
        bit_pattern = create_euclidean_rhythm(rand_steps, 16);
        break;
    case (1):
        bit_pattern = markov_bit_pattern_generate(0);
        break;
    }
    char binnum[17] = {0};
    short_to_char(bit_pattern, binnum);

    apply_short_to_midi_pattern_sub_pattern(bit_pattern, start_idx, pattern_len,
                                            midi_pattern);

    if (rand() % 100 > (100 - j->pct_probability))
    {
        depth++;
        _juggler_recursive_generation(j, start_idx, pattern_len / 2, depth,
                                      midi_pattern);
        _juggler_recursive_generation(j, start_idx + pattern_len / 2,
                                      pattern_len / 2, depth, midi_pattern);
    }
}

void juggler_generate(void *self, void *data)
{
    juggler *j = (juggler *)self;
    midi_event *midi_pattern = (midi_event *)data;
    _juggler_recursive_generation(j, 0, PPBAR, 0, midi_pattern);
}

void juggler_set_debug(void *self, bool b)
{
    juggler *j = (juggler *)self;
    j->debug = b;
}

void juggler_event_notify(void *self, unsigned int event_type) {}

void juggler_set_style(juggler *j, unsigned int style)
{
    if (style < NUM_JUGGLER_STYLES)
        j->juggler_style = style;
}

void juggler_set_max_depth(juggler *j, int depth)
{
    if (depth > 0)
        j->max_depth = depth;
}
void juggler_set_pct_probability(juggler *j, int pct_probability)
{
    if (pct_probability >= 0 && pct_probability <= 100)
        j->pct_probability = pct_probability;
}
