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

    return b;
}


double bytes_gen_next(void *self)
{
    bytebeat *b = (bytebeat*) self;
    printf("Bytes beat! %s\n", b->pattern);
    return interpreter(b->pattern);
}

void   bytes_status(void *self, char *ss);

