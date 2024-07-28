#pragma once

#include <defjams.h>
#include <mixer.h>

void midi_launch_init(Mixer *mixr);
void midi_set_destination(Mixer *mixr, int soundgen_num);
