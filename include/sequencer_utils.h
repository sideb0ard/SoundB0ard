#pragma once

#include <wchar.h>

#include <defjams.h>
#include <step_sequencer.h>

int shift_bits_to_leftmost_position(int num, int num_of_bits_to_align_with);
void char_binary_version_of_short(int num, char bin_num[17]);
// void char_binary_version_of_pattern(seq_pattern p, char bin_num[17]);
// void wchar_binary_version_of_pattern(seq_pattern p, wchar_t bin_num[17]);
int pattern_as_int_representation(midi_pattern p);
void wchar_binary_version_of_short(int num, wchar_t bin_num[17]);
void convert_bit_pattern_to_midi_pattern(int bitpattern, int bitpattern_len,
                                         midi_event *pattern,
                                         int division, int offset);
void print_pattern(int *pattern_array, int len_pattern_array);
