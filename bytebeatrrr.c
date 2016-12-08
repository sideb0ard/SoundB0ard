#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bytebeatrrr.h"
#include "bytebeat/interpreter.h"

bytebeat *new_bytebeat(char *pattern)
{
    bytebeat *b = calloc(1, sizeof(bytebeat));
    strncpy(b->pattern, pattern, 255);

    b->sound_generator.gennext = &bytes_gen_next;
    b->sound_generator.status = &bytes_status;

    return b;
}


double bytes_gen_next(void *self)
{
    bytebeat *b = (bytebeat*) self;
    //printf("Bytes beat! %s\n", b->pattern);
    char val = interpreter(b->pattern);
    double scaled_val = 2.0 / 256 * val;
    //printf("SCALED! %f\n", scale_val);
    return scaled_val;
}

void bytes_status(void *self, char *status_string)
{
    bytebeat *b = (bytebeat*) self;
    snprintf(status_string, 119, COOL_COLOR_GREEN "[%s]\tvol: %.2lfs"
                                 ANSI_COLOR_RESET, b->pattern, 0.5);
}
