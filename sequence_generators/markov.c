#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "markov.h"
#include "mixer.h"
#include "sequence_generator.h"
#include "utils.h"

extern mixer *mixr;
static char *s_markov_types[] = {"GARAGE", "HIPHOP", "HATS"};

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

sequence_generator *new_markov(unsigned int type)
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
    return (sequence_generator *)m;
}

void markov_status(void *self, wchar_t *wstring)
{
    markov *m = (markov *)self;
    swprintf(wstring, MAX_PS_STRING_SZ,
             L"[" WANSI_COLOR_WHITE "MARKOV SEQUENCE GEN ] - " WCOOL_COLOR_PINK
             "type:%s\n",
             s_markov_types[m->markov_type]);
}

static int rand_percent()
{
    int randy = rand() % 100;
    return randy;
}

uint16_t markov_generate(void *self, void *data)
{
    markov *m = (markov *)self;
    int first = 0;
    int second = 0;
    int third = 0;
    int fourth = 0;

    uint16_t pattern = 0;
    switch (m->markov_type)
    {
    case (GARAGE):
        if (rand_percent() > 10)
            pattern |= BEAT0;

        second = rand_percent();
        if (second > 70 && second < 80)
            pattern |= BEAT4;
        else if (second >= 80)
            pattern |= BEAT7;

        third = rand_percent();
        if (third > 50 && third < 75)
            pattern |= BEAT9;
        if (third >= 75)
            pattern |= BEAT10;
        else if (third >= 90)
            pattern |= BEAT11;

        fourth = rand_percent();
        if (third > 50 && third < 75)
            pattern |= BEAT13;
        else if (third >= 75 && third < 90)
            pattern |= BEAT14;
        else if (third >= 90)
            pattern |= BEAT15;

        break;
    case (HIPHOP):
        first = rand_percent();
        if (first > 10)
            pattern |= BEAT0;
        if (first > 70 && first < 90)
            pattern |= BEAT2;
        else if (first >= 90)
            pattern |= BEAT3;

        second = rand_percent();
        if (second > 70 && second < 80)
            pattern |= BEAT4;
        else if (second >= 80)
            pattern |= BEAT6;
        if (second > 90)
            pattern |= BEAT7;

        third = rand_percent();
        if (third < 10)
            pattern |= BEAT8;
        if (third > 50 && third < 75)
            pattern |= BEAT9;
        else if (third >= 75 && third < 90)
            pattern |= BEAT10;
        else if (third >= 90)
            pattern |= BEAT11;

        fourth = rand_percent();
        if (third > 50 && third < 75)
            pattern |= BEAT13;
        else if (third >= 75 && third < 90)
            pattern |= BEAT14;
        else if (third >= 90)
            pattern |= BEAT15;

        break;
    case (HATS):
        for (int i = 0; i < 16; i++)
            if (rand_percent() > 40)
                pattern |= 1 << (15 - i);
        break;
    }
    if (pattern == 0)
        return markov_generate(self, data);
    return pattern;
}

void markov_set_debug(void *self, bool b) {}

void markov_event_notify(void *self, unsigned int event_type) {}
