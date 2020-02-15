#pragma once

#include <defjams.h>
#include <mixer.h>

void midi_launch_init(mixer *mixr);
void midi_set_destination(mixer *mixr, int soundgen_num);
