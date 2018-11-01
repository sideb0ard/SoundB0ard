#include <string.h>
#include <stdlib.h>

#include <pattern_utils.h>
#include <utils.h>
#include <mixer.h>

extern mixer *mixr;
extern const wchar_t *sparkchars;

void clear_midi_pattern(midi_event *pattern)
{
    memset(pattern, 0, sizeof(midi_event) * PPBAR);
}

void midi_pattern_add_event(midi_event *pattern, int midi_tick, midi_event ev)
{
    int target_tick = midi_tick;
    while (pattern[target_tick].event_type)
    {
        if (mixr->debug_mode)
            printf("Gotsz a tick already - bump!\n");
        target_tick++;
        if (target_tick == PPBAR) // wrap around
            target_tick = 0;
    }
    pattern[target_tick] = ev;
}

bool is_midi_event_in_pattern_range(int start_tick, int end_tick, midi_pattern pattern)
{
    for (int i = start_tick; i < end_tick; i++)
    {
        if (pattern[i].event_type == MIDI_ON)
            return true;
    }
    return false;
}

void midi_pattern_add_triplet(midi_event *pattern, unsigned int quarter)
{
    if (quarter > 3) // no can do!
        return;

    int offset = quarter * PPQN;
    // magic numbers - euclidean position for 3/16
    // is 0, 5, 10 and PPQN / 16 == 60
    // therefore 0*60, 5*60 and 10*60
    int pos[3] = {offset, offset + 300, offset + 600};

    memset(pattern + offset, 0, sizeof(midi_event) * PPQN);
    for (int i = 0; i < 3; i++)
    {
        pattern[pos[i]].event_type = MIDI_ON;
        pattern[pos[i]].data2 = DEFAULT_VELOCITY;
    }
}



uint16_t midi_pattern_to_short(midi_event *pattern)
{
    uint16_t bit_pattern = 0;
    for (int i = 0; i < 16; i++)
    {
        int start = i * PPSIXTEENTH;
        if (is_midi_event_in_pattern_range(start, start + PPSIXTEENTH, pattern))
        {
            bit_pattern |= 1 << (15-i);
        }
    }
    return bit_pattern;
}

void midi_pattern_to_widechar(midi_event *pattern, wchar_t *patternstr)
{
    int cur_sixteenth = 0;
    for (int i = 0; i < PPBAR; i += PPSIXTEENTH)
    {
        patternstr[cur_sixteenth] = sparkchars[0];
        patternstr[cur_sixteenth + 1] = sparkchars[0];
        for (int j = i; j < (i + PPSIXTEENTH); j++)
        {
            if (pattern[j].event_type == MIDI_ON)
            {
                int sparklevel = scaleybum(0, 127, 0, 5, pattern[j].data2);
                patternstr[cur_sixteenth] = sparkchars[sparklevel];
                patternstr[cur_sixteenth + 1] = sparkchars[sparklevel];
            }
        }
        cur_sixteenth += 2;
    }
}

void short_to_midi_pattern(uint16_t bit_pattern, midi_event *dest_pattern)
{
}

void short_to_char(uint16_t num, char bin_num[17])
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

//void print_parceled_pattern(midi_event *pattern)
//{
//    printf("PATTERN!\n");
//    bool found = false;
//    for (int i = 0; i < PPBAR; i += PPSIXTEENTH)
//    {
//        // printf("%d", pattern.pattern[i]);
//        found = false;
//        for (int j = 0; j < PPSIXTEENTH && !found; j++)
//        {
//            if (pattern[i + j].event_type == MIDI_ON)
//            {
//                printf("1");
//                found = true;
//            }
//        }
//        if (!found)
//            printf("0");
//    }
//    puts("\n");
//}

void convert_bit_pattern_to_midi_pattern(int bitpattern, int bitpattern_len,
                                         midi_event *pattern, int division,
                                         int offset)
{

    clear_midi_pattern(pattern);

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
        }
    }
}

int shift_bits_to_leftmost_position(uint16_t num, int num_of_bits_to_align_with)
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
