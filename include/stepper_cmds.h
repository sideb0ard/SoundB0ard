#pragma once

#include <defjams.h>

bool parse_stepper_cmd(int num_wurds, char wurds[][SIZE_OF_WURD]);
bool parse_dx_command(int soundgen_num, char wurds[][SIZE_OF_WURD],
                      int num_wurds);
