#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "markov.h"
#include "mixer.h"
#include "pattern_generator.h"
#include "pattern_utils.h"
#include "utils.h"
#include <euclidean.h>

extern mixer *mixr;
static char *s_markov_types[] = {"GARAGE",     "HIPHOP", "HATS",
                                 "HATS_MASK",  "CLAPS",  "RAGGA_KICK",
                                 "RAGGA_SNARE"};

#define BEAT0 32768
#define BEAT1 16384
#define BEAT2 8192
#define BEAT3 4096
#define BEAT4 2048
#define BEAT5 1024
#define BEAT6 512
#define BEAT7 256
#define BEAT8 128
#define BEAT9 64
#define BEAT10 32
#define BEAT11 16
#define BEAT12 8
#define BEAT13 4
#define BEAT14 2
#define BEAT15 1

pattern_generator *new_markov(unsigned int type)
{
    markov *m = calloc(1, sizeof(markov));
    if (!m)
    {
        printf("WOOF!\n");
        return NULL;
    }
    m->markov_type = type;

    m->sg.status = &markov_status;
    m->sg.generate = &markov_generate;
    m->sg.event_notify = &markov_event_notify;
    m->sg.set_debug = &markov_set_debug;
    m->sg.type = MARKOV;
    return (pattern_generator *)m;
}

void markov_status(void *self, wchar_t *wstring)
{
    markov *m = (markov *)self;
    swprintf(wstring, MAX_STATIC_STRING_SZ,
             L"[" WANSI_COLOR_WHITE "MARKOV PATTERN GEN ] - " WCOOL_COLOR_PINK
             "type:(%d)%s\n",
             m->markov_type, s_markov_types[m->markov_type]);
}

static int rand_percent()
{
    int randy = rand() % 100;
    return randy;
}

void markov_generate(void *self, void *data)
{
    markov *m = (markov *)self;
    midi_event *midi_pattern = (midi_event*) data;

    int first = 0;
    int second = 0;
    int third = 0;
    int fourth = 0;

    uint16_t bit_pattern = 0;
    switch (m->markov_type)
    {
    case (GARAGE):
        if (rand_percent() > 10)
            bit_pattern |= BEAT0;

        second = rand_percent();
        if (second > 70)
            bit_pattern |= BEAT7;
        if (second >= 90)
            bit_pattern |= BEAT5;

        third = rand_percent();
        if (third > 10)
            bit_pattern |= BEAT8;
        if (third > 50)
            bit_pattern |= BEAT10;
        if (third >= 75)
            bit_pattern |= BEAT11;
        else if (third >= 90)
            bit_pattern |= BEAT9;

        fourth = rand_percent();
        if (third > 60)
            bit_pattern |= BEAT13;
        if (third >= 90)
            bit_pattern |= BEAT15;

        break;
    case (HIPHOP):
        first = rand_percent();
        if (first > 10)
            bit_pattern |= BEAT0;
        if (first > 60)
            bit_pattern |= BEAT3;
        if (first >= 90)
            bit_pattern |= BEAT2;

        second = rand_percent();
        if (second > 80)
            bit_pattern |= BEAT6;

        third = rand_percent();
        if (third > 10)
            bit_pattern |= BEAT8;
        if (third > 40)
            bit_pattern |= BEAT9;
        if (third > 90)
            bit_pattern |= BEAT11;
        else if (third >= 60)
            bit_pattern |= BEAT10;

        fourth = rand_percent();
        if (third > 80)
            bit_pattern |= BEAT13;
        if (third >= 90)
            bit_pattern |= BEAT15;

        break;
    case (HATS):
        for (int i = 0; i < 16; i++)
            if (rand_percent() > 40)
                bit_pattern |= 1 << (15 - i);
        break;
    case (HATS_MASK):
        for (int i = 10; i < 16; i++)
            if (rand_percent() > 40)
                bit_pattern |= 1 << (15 - i);
        break;
    case (CLAPS):
        if (rand_percent() > 90)
        {
            int num_hits = rand() % 7;
            // bit_pattern = create_euclidean_rhythm(num_hits, 16);
            bit_pattern = create_euclidean_rhythm(0, 16);
        }
        else
        {
            if (rand_percent() > 10)
                bit_pattern |= BEAT4;
            if (rand_percent() > 90)
                bit_pattern |= BEAT7;
            if (rand_percent() > 95)
                bit_pattern |= BEAT10;
            if (rand_percent() > 10)
                bit_pattern |= BEAT12;
        }
        break;
    case (RAGGA_BEAT):
        first = rand_percent();
        if (first > 10)
            bit_pattern |= BEAT0;
        if (first > 20)
            bit_pattern |= BEAT3;
        if (first >= 90)
            bit_pattern |= BEAT2;

        second = rand_percent();
        if (second > 90)
            bit_pattern |= BEAT7;

        third = rand_percent();
        if (third > 10)
            bit_pattern |= BEAT8;
        if (third > 20)
            bit_pattern |= BEAT11;

        fourth = rand_percent();
        if (third > 80)
            bit_pattern |= BEAT13;
        if (third >= 90)
            bit_pattern |= BEAT15;
        break;

    case (RAGGA_SNARE):
        first = rand_percent();
        if (first > 10)
            bit_pattern |= BEAT6;
        if (first > 20)
            bit_pattern |= BEAT14;
        break;
    }
    if (bit_pattern == 0)
        markov_generate(self, data);

    apply_short_to_midi_pattern(bit_pattern, midi_pattern);
}

void markov_set_debug(void *self, bool b) {}

void markov_event_notify(void *self, unsigned int event_type) {}

void markov_set_type(markov *m, unsigned int type)
{
    if (type < NUM_MARKOV_STYLES)
        m->markov_type = type;
}
