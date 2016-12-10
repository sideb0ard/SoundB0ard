#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bytebeatrrr.h"
#include "bytebeat/interpreter.h"
#include "mixer.h"

extern mixer *mixr;

bytebeat *new_bytebeat(char *pattern)
{
    bytebeat *b = calloc(1, sizeof(bytebeat));
    strncpy(b->pattern, pattern, 255);

    b->rpn_stack = new_rpn_stack(pattern);

    b->sound_generator.gennext = &bytes_gen_next;
    b->sound_generator.status = &bytes_status;
    b->sound_generator.setvol = &bytes_setvol;

    b->vol = 0.025;

    // TODO stack destroy
    // stack_Destroy(rpn_stack);

    return b;
}


double bytes_gen_next(void *self)
{
    bytebeat *b = (bytebeat*) self;
    // //printf("Bytes beat! %s\n", b->pattern);
    char val = interpreter(b->rpn_stack);
    double scaled_val = 2.0 / 256 * val;
    //printf("SCALED! %f\n", scale_val);
    return scaled_val * b->vol;
}

void bytes_status(void *self, char *status_string)
{
    bytebeat *b = (bytebeat*) self;
    snprintf(status_string, 119, COOL_COLOR_GREEN "[%s]\tvol: %.2lfs"
                                 ANSI_COLOR_RESET, b->pattern, b->vol);
}


void bytes_setvol(void *self, double v)
{
    bytebeat *b = (bytebeat *)self;
    if (v < 0.0 || v > 1.0) {
        return;
    }
    b->vol = v;
}

