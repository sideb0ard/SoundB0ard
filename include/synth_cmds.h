#pragma once

#include <defjams.h>

bool parse_synth_cmd(int num_wurds, char wurds[][SIZE_OF_WURD]);
bool parse_sequence_engine_cmd(int soungen_num, int target_pattern_num,
                               char wurds[][SIZE_OF_WURD], int num_wurds);
