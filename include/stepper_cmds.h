#pragma once

#include <defjams.h>
#include <drumsampler.h>
#include <drumsynth.h>

bool parse_stepper_cmd(int num_wurds, char wurds[][SIZE_OF_WURD]);
void parse_drumsampler_cmd(drumsampler *ds, char wurds[][SIZE_OF_WURD],
                           int num_wurds);
void parse_drumsynth_cmd(drumsynth *ds, char wurds[][SIZE_OF_WURD],
                         int num_wurds);
