#include <stdlib.h>

#include "bitwize.h"
#include "defjams.h"
#include "mixer.h"
#include "sound_generator.h"

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

    p_bitwize->sound_generator.gennext = &bitwize_gennext;
    p_bitwize->sound_generator.status = &bitwize_status;
    // TODO
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
    BITWIZE *p_bitwize = (BITWIZE *)self;
    int t = p_bitwize->incr++;
    double vol = p_bitwize->vol;
    int pattern = p_bitwize->pattern;
    char val = bitwize_process(pattern, t);

    double scale_val = (2.0 / 256 * val);
    printf("VAL: %d, SCALE_VAL: %f\n", val, scale_val);
    // scale_val = effector(&p_bitwize->sound_generator, scale_val);
    // scale_val = envelopor(&p_bitwize->sound_generator, scale_val);
    // return scale_val * vol;
    return 0;
}

void bitwize_status(void *self, char *status_string)
{
    BITWIZE *p_bitwize = self;
    snprintf(status_string, 119,
             ANSI_COLOR_MAGENTA "BWIZE vol: %f pattern: %d" ANSI_COLOR_RESET,
             p_bitwize->vol, p_bitwize->pattern);
}
