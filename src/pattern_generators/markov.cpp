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
static char *s_markov_types[] = {
    (char *)"CLAP2",      (char *)"KICK2",      (char *)"CLAPS",
    (char *)"GARAGE",     (char *)"HATS",       (char *)"HATS2",
    (char *)"HATS_MASK",  (char *)"HIPHOP",     (char *)"HOUSE",
    (char *)"RAGGA_KICK", (char *)"RAGGA_SNARE"};

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
    markov *m = (markov *)calloc(1, sizeof(markov));
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

void markov_pattern_generate(unsigned int markov_type, midi_event *midi_pattern)
{
    int first = 0;
    int second = 0;
    int third = 0;
    int fourth = 0;

    midi_event ev = {};
    ev.data1 = get_midi_note_from_mixer_key(mixr->timing_info.key,
                                            mixr->timing_info.octave);
    ev.event_type = MIDI_ON;
    ev.source = 0;

    if (markov_type == CLAP2)
    {
        int shifty1 = 1680 + (rand() % 80);
        int shifty2 = 2160 + (rand() % 80);
        int midi_timings[5] = {960, shifty1, 1920, shifty2, 2880};
        for (int i = 0; i < 5; i++)
        {
            if (rand() % 100 > 10)
            {
                if (midi_timings[i] % 960 == 0)
                    ev.data2 = 128; // velocity
                else
                {
                    int rand_velocity = (rand() % 50) + 70;
                    ev.data2 = rand_velocity;
                }
                midi_pattern[midi_timings[i]] = ev;
            }
        }
    }
    else if (markov_type == KICK2)
    {
        if (rand() % 100 > 10)
        {
            ev.data2 = 128; // velocity
            midi_pattern[0] = ev;
        }
        if (rand() % 100 > 50)
        {
            ev.data2 = (rand() % 50) + 70;
            midi_pattern[480] = ev;
        }
        if (rand() % 100 > 10)
        {
            ev.data2 = 128; // velocity
            midi_pattern[1200] = ev;
        }
        if (rand() % 100 > 95)
        {
            ev.data2 = 70; // velocity
            midi_pattern[3700] = ev;
        }
    }
    else
    {
        uint16_t bit_pattern = 0;
        switch (markov_type)
        {
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
                int randy = rand_percent();
                if (randy > 95)
                    bit_pattern |= BEAT10;
                else if (randy > 92)
                    bit_pattern |= BEAT11;
                if (rand_percent() > 10)
                    bit_pattern |= BEAT12;
            }
            break;
        case (GARAGE):
            if (rand_percent() > 10)
                bit_pattern |= BEAT0;

            second = rand_percent();
            if (second > 93)
                bit_pattern |= BEAT7;
            if (second >= 97)
                bit_pattern |= BEAT5;

            third = rand_percent();
            if (third > 10)
                bit_pattern |= BEAT8;
            if (third > 94)
                bit_pattern |= BEAT10;
            if (third >= 10)
                bit_pattern |= BEAT11;
            else if (third >= 98)
                bit_pattern |= BEAT9;

            fourth = rand_percent();
            if (third > 96)
                bit_pattern |= BEAT13;
            if (third >= 98)
                bit_pattern |= BEAT15;

            break;
        case (HATS):
            for (int i = 0; i < 16; i++)
                if (rand_percent() > 40)
                    bit_pattern |= 1 << (15 - i);
            break;
        case (HATS2):
            for (int i = 0; i < 16; i++)
            {
                if (i % 4 == 0)
                {
                    if (rand_percent() > 40)
                        bit_pattern |= 1 << (15 - i);
                }
                else if ((i % 4) - 3 == 0)
                {
                    if (rand_percent() > 40)
                        bit_pattern |= 1 << (15 - i);
                }
            }
            break;
        case (HATS_MASK):
            for (int i = 10; i < 16; i++)
                if (rand_percent() > 40)
                    bit_pattern |= 1 << (15 - i);
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
        case (HOUSE):
            first = rand_percent();
            if (first > 10)
                bit_pattern |= BEAT0;
            if (first > 90)
                bit_pattern |= BEAT3;

            second = rand_percent();
            if (second > 10)
                bit_pattern |= BEAT4;

            third = rand_percent();
            if (third > 10)
                bit_pattern |= BEAT8;

            fourth = rand_percent();
            if (third > 20)
                bit_pattern |= BEAT12;
            if (third >= 90)
                bit_pattern |= BEAT15;

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
            markov_pattern_generate(markov_type, midi_pattern);

        apply_short_to_midi_pattern(bit_pattern, midi_pattern);
    }
}

void markov_generate(void *self, void *data)
{
    markov *m = (markov *)self;
    midi_event *midi_pattern = (midi_event *)data;
    markov_pattern_generate(m->markov_type, midi_pattern);
}
void markov_set_debug(void *self, bool b) {}

void markov_event_notify(void *self, broadcast_event event) {}

void markov_set_type(markov *m, unsigned int type)
{
    if (type < NUM_MARKOV_STYLES)
        m->markov_type = type;
}
