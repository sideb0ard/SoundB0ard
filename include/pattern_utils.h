#include <defjams.h>

#include <stdint.h>
#include <wchar.h>

// MIDI PATTERNS
void clear_midi_pattern(midi_event *pattern);
void midi_pattern_add_event(midi_event *pattern, int midi_tick, midi_event ev);
bool is_midi_event_in_pattern_range(int start_tick, int end_tick,
                                    midi_pattern pattern);
void midi_pattern_add_triplet(midi_event *pattern, unsigned int quarter);
// void print_pattern(int *pattern_array, int len_pattern_array);

// TRANSFORMS
uint16_t midi_pattern_to_short(midi_event *pattern);
void midi_pattern_to_widechar(midi_event *pattern, wchar_t *patternstr);

void apply_short_to_midi_pattern(uint16_t bit_pattern,
                                 midi_event *dest_pattern);
void set_pattern_to_self_destruct(midi_event *pattern);
void short_to_char(uint16_t num, char bin_num[17]);

// void char_binary_version_of_pattern(seq_pattern p, char bin_num[17]);
// void wchar_binary_version_of_pattern(seq_pattern p, wchar_t bin_num[17]);
//

int shift_bits_to_leftmost_position(uint16_t num,
                                    int num_of_bits_to_align_with);

void pattern_replace(midi_event *src_pattern, midi_event *dst_pattern);
void pattern_apply_swing(midi_event *pattern, int swing_setting);
