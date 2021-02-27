#pragma once

#include <defjams.h>

#include <stdint.h>
#include <value_generator.h>
#include <wchar.h>

// MIDI PATTERNS
void clear_midi_pattern(midi_event *pattern);
void midi_pattern_add_event(midi_event *pattern, int midi_tick, midi_event ev);
bool is_midi_event_in_pattern_range(int start_tick, int end_tick,
                                    midi_pattern pattern);
void midi_pattern_add_triplet(midi_event *pattern, unsigned int quarter);
void midi_pattern_clear_events_from(midi_event *pattern, int start_tick);
// void print_pattern(int *pattern_array, int len_pattern_array);

// TRANSFORMS
uint16_t midi_pattern_to_short(midi_event *pattern);
void midi_pattern_to_widechar(midi_event *pattern, wchar_t *patternstr);

void apply_short_to_midi_pattern(uint16_t bit_pattern,
                                 midi_event *dest_pattern);
void apply_short_to_midi_pattern_sub_pattern(uint16_t bit_pattern,
                                             int start_idx, int pattern_len,
                                             midi_event *dest_pattern);
void set_pattern_to_self_destruct(midi_event *pattern);
void short_to_char(uint16_t num, char bin_num[17]);
uint16_t char_to_short(char bin_num[17]);

int shift_bits_to_leftmost_position(uint16_t num,
                                    int num_of_bits_to_align_with);

void pattern_replace(midi_event *src_pattern, midi_event *dst_pattern);
void pattern_apply_swing(midi_event *pattern, int swing_setting);
void pattern_check_idx(int *idx, int pattern_len);
void pattern_apply_values(value_generator *vg, midi_event *pattern);

// POST DEC 2019 STUFF

std::string PatternPrint(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events);
