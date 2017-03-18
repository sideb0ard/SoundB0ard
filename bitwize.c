#include <stdlib.h>

#include "bitwize.h"
#include "defjams.h"
#include "mixer.h"
#include "sound_generator.h"
#include "utils.h"

extern mixer *mixr;

BITWIZE *new_bitwize(int pattern)
{
    BITWIZE *p_bitwize;
    p_bitwize = (BITWIZE *)calloc(1, sizeof(BITWIZE));
    if (p_bitwize == NULL)
        return NULL;
    p_bitwize->vol = 0.0;
    p_bitwize->incr = 0;
    p_bitwize->pattern = pattern;
    printf("NEW BITWIZET!\n");

    p_bitwize->m_lfo = lfo_new();
    p_bitwize->sound_generator.gennext = &bitwize_gennext;
    p_bitwize->sound_generator.status = &bitwize_status;
    p_bitwize->sound_generator.getvol = &bitwize_getvol;
    p_bitwize->sound_generator.setvol = &bitwize_setvol;
    p_bitwize->sound_generator.type = BITWIZE_TYPE;

    return p_bitwize;
}

double bitwize_getvol(void *self)
{
    BITWIZE *p_bitwize = (BITWIZE *)self;
    return p_bitwize->vol;
}

void bitwize_setvol(void *self, double v)
{
    BITWIZE *p_bitwize = (BITWIZE *)self;
    if (v < 0.0 || v > 0.5) {
        printf("Too loud! try 0.4\n");
        return;
    }
    p_bitwize->vol = v;
}

char bitwize_process(int pattern, int t)
{
    switch (pattern) {
    case 0:
        return t * ((t >> 9 | t >> 13) & 25 & t >> 6);
    case 1:
        return (t >> 7 | t | t >> 6) * 10 + 4 * ((t & (t >> 13)) | t >> 6);
    case 2:
        return (t * (t >> 5 | t >> 8)) >> (t >> 16);
    case 3:
        return (t * (t >> 3 | t >> 4)) >> (t >> 7);
    case 4:
        return (t * (t >> 13 | t >> 4)) >> (t >> 3);
    default:
        return (t * (t >> 13 | t >> 4)) >> (t >> 3);
    }
}

double bitwize_gennext(void *self)
{
    BITWIZE *b = (BITWIZE *)self;

    int t = b->incr++;
    int pattern = b->pattern;
    char val = bitwize_process(pattern, t);
    double scale_val = (2.0 / 256 * val);
    // double scaled = scaleybum(-128, 127, -1.0, 0.9, val);

    double yn = 0;
    double yqn = 0;
    yn = lfo_do_oscillate((oscillator *)b->m_lfo, &yqn);

    int tick = mixr->cur_sample % 9600000000;
    // int tick = mixr->sixteenth_note_tick;

    if (b->tick != tick) {
        b->tick = tick;
        b->current_val = scale_val;
    }

    // printf("VAL: %d, SCALE_VAL: %f SCALED: %f CURRENT: %f \n", val,
    // scale_val, scaled, b->current_val);

    double return_val = b->current_val;

    return_val = effector(&b->sound_generator, return_val);
    return_val = envelopor(&b->sound_generator, return_val);

    ////printf("VAL:%f\n", return_val);
    // return return_val * 0.4 * yn;
    return return_val * 0.4;
}

void bitwize_status(void *self, wchar_t *status_string)
{
    BITWIZE *p_bitwize = (BITWIZE *)self;
    swprintf(status_string, MAX_PS_STRING_SZ,
             WANSI_COLOR_MAGENTA "BWIZE vol: %f pattern: %d" ANSI_COLOR_RESET,
             p_bitwize->vol, p_bitwize->pattern);
}
