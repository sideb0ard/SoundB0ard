#include <math.h>
#include <stdio.h>
#include <string.h>

#include "bitshift.h"
#include "mixer.h"
#include "sequencer_utils.h"
#include "utils.h"
#include <euclidean.h>
#include <pattern_parser.h>

extern mixer *mixr;
extern const wchar_t *sparkchars;

void convert_bit_pattern_to_midi_pattern(int bitpattern, int bitpattern_len,
                                         midi_event *pattern, int division,
                                         int offset)
{

    clear_pattern(pattern);

    int pulses = PPSIXTEENTH / division;
    // bool triplet_set = false;

    for (int i = 0; i < bitpattern_len; i++)
    {
        int shift_by = bitpattern_len - 1 - i;
        int idx = offset + (i * pulses);
        if (idx >= PPBAR)
            idx = idx - PPBAR;
        if (bitpattern & 1 << shift_by)
        {
            // printf("IDX:%d!\n", idx);
            pattern[idx].event_type = MIDI_ON;
            if (i == 0 || i == 8)
                pattern[idx].data2 = 128;
            else
            {
                int rand_velocity = (rand() % 50) + 70;
                pattern[idx].data2 = rand_velocity;
            }
            // if (rand() % 100 > 95 && !triplet_set)
            //{
            //    uint16_t euc = create_euclidean_rhythm(3, 16);
            //    int small_offset = i * pulses;
            //    // printf("Ooh, random triplet!\n");
            //    convert_bit_pattern_to_midi_pattern(euc, 16, pattern, 16,
            //                                        small_offset);
            //    triplet_set = true;
            //}
        }
    }
}

int shift_bits_to_leftmost_position(int num, int num_of_bits_to_align_with)
{
    int first_position = 0;
    for (int i = num_of_bits_to_align_with; i >= 0; i--)
    {
        if (num & (1 << i))
        {
            first_position = i;
            break;
        }
    }
    int bitshift_by = num_of_bits_to_align_with - (first_position + 1);
    int ret_num = num << bitshift_by;
    // print_binary_version_of_int(num);
    // print_binary_version_of_int(ret_num);

    return ret_num;
}

void char_binary_version_of_short(int num, char bin_num[17])
{
    for (int i = 15; i >= 0; i--)
    {
        if (num & 1 << i)
            bin_num[15 - i] = '1';
        else
            bin_num[15 - i] = '0';
    }
    bin_num[16] = '\0';
}

void print_pattern(int *pattern_array, int len_pattern_array)
{
    printf("PP\n\n");
    for (int i = 0; i < len_pattern_array; i++)
    {
        if (pattern_array[i])
        {
            printf("PATTERN ON: %d\n", i);
            printf("PATTERN SIZTEENTH: %d\n", i / PPSIXTEENTH);
        }
    }
}

int pattern_as_int_representation(midi_pattern p)
{
    int pattern_as_int = 0;
    for (int i = 0; i < 16; i++)
    {
        int start = i * PPSIXTEENTH;
        if (is_midi_event_in_range(start, start + PPSIXTEENTH, p))
        {
            int cur_bit = pow(2, 15 - i);
            pattern_as_int = pattern_as_int | cur_bit;
        }
    }
    return pattern_as_int;
}
