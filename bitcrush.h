#pragma once

#include "defjams.h"
#include "fx.h"

typedef struct bitcrush
{
    fx m_fx;
    int bitdepth;
    int bitrate;
} bitcrush;

bitcrush *new_bitcrush(void);
void bitcrush_init(bitcrush *bc);

void bitcrush_status(void *self, char *string);
double bitcrush_process_audio(void *self, double input);

void bitcrush_set_bitdepth(bitcrush *bc, int val);
void bitcrush_set_bitrate(bitcrush *bc, int val);
