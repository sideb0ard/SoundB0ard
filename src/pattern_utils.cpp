#include <stdlib.h>
#include <string.h>

#include <iostream>

#include <midimaaan.h>
#include <mixer.h>
#include <pattern_utils.h>
#include <utils.h>
#include <value_generator.h>

extern Mixer *mixr;
extern const wchar_t *sparkchars;

const int MAX_ADD_ATTEMPTS = 5;

void clear_midi_pattern(midi_event *pattern)
{
    memset(pattern, 0, sizeof(midi_event) * PPBAR);
}

void midi_pattern_clear_events_from(midi_event *pattern, int start_tick)
{
    if (start_tick < PPBAR)
    {
        int num_ticks = PPBAR - start_tick;
        memset(&pattern[start_tick], 0, sizeof(midi_event) * num_ticks);
    }
}

void midi_pattern_add_event(midi_event *pattern, int midi_tick, midi_event ev)
{
    int target_tick = midi_tick % PPBAR;
    int attempt_no = 0;
    while (pattern[target_tick].event_type && attempt_no < MAX_ADD_ATTEMPTS)
    {
        if (mixr->debug_mode)
            printf("Gotsz a tick already - bump!\n");
        target_tick++;
        target_tick = target_tick % PPBAR; // wrap around
        attempt_no++;
    }
    if (attempt_no < MAX_ADD_ATTEMPTS)
        pattern[target_tick] = ev;
}

bool is_midi_event_in_pattern_range(int start_tick, int end_tick,
                                    midi_pattern pattern)
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
        pattern[pos[i]].data1 = get_midi_note_from_mixer_key(
            mixr->timing_info.key, mixr->timing_info.octave);
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
            bit_pattern |= 1 << (15 - i);
        }
    }
    return bit_pattern;
}

void midi_pattern_to_widechar(midi_event *pattern,
                              wchar_t *patternstr) // len 33
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

uint16_t char_to_short(char bin_num[17])
{
    uint16_t num = 0;
    for (int i = 0; i < 17; i++)
    {
        if (bin_num[i] == '1')
            num |= 1 << (15 - i);
    }

    return num;
}

void apply_short_to_midi_pattern_sub_pattern(uint16_t bit_pattern,
                                             int start_idx, int pattern_len,
                                             midi_event *dest_pattern)
{
    int ticks_per_quart = pattern_len / 16;
    // printf("START IDX:%d Len:%d Ticks:%d\n", start_idx, pattern_len,
    // ticks_per_quart);
    for (int i = 0; i < 15; i++)
    {
        if (bit_pattern & (1 << (15 - i)))
        {
            int midi_pos = start_idx + (i * ticks_per_quart);
            pattern_check_idx(&midi_pos, PPBAR);
            int hold_time_ms = (rand() % 200) + 130;

            midi_event *ev = &dest_pattern[midi_pos];
            ev->data1 = get_midi_note_from_mixer_key(mixr->timing_info.key,
                                                     mixr->timing_info.octave);
            ev->event_type = MIDI_ON;
            ev->source = 0;
            ev->hold = hold_time_ms;

            if (i == 0 || i == 8)
                ev->data2 = 128; // velocity
            else
            {
                int rand_velocity = (rand() % 50) + 70;
                ev->data2 = rand_velocity;
            }
        }
    }
}

void apply_short_to_midi_pattern(uint16_t bit_pattern, midi_event *dest_pattern)
{
    clear_midi_pattern(dest_pattern);
    for (int i = 0; i < 15; i++)
    {
        if (bit_pattern & (1 << (15 - i)))
        {
            int midi_pos = i * PPSIXTEENTH;
            int hold_time_ms = (rand() % 2000) + 130;

            midi_event *ev = &dest_pattern[midi_pos];
            ev->data1 = get_midi_note_from_mixer_key(mixr->timing_info.key,
                                                     mixr->timing_info.octave);
            ev->event_type = MIDI_ON;
            ev->source = 0;
            ev->hold = hold_time_ms;

            if (i % 4 == 0)
                ev->data2 = 128; // velocity
            else
            {
                int rand_velocity = (rand() % 80) + 38;
                ev->data2 = rand_velocity;
            }
        }
    }
    if (rand() % 100 > 95)
    {
        int quart = 2;
        int randy = rand() % 100;
        if (randy > 75)
            quart = 1;
        else if (randy > 50)
            quart = 3;
        midi_pattern_add_triplet(dest_pattern, quart);
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
    return ret_num;
}

void set_pattern_to_self_destruct(midi_event *pattern)
{
    for (int i = 0; i < PPBAR; i++)
    {
        midi_event *ev = &pattern[i];
        if (ev->event_type == MIDI_ON || ev->event_type == MIDI_OFF)
            ev->delete_after_use = true;
    }
}

void pattern_replace(midi_event *src_pattern, midi_event *dst_pattern)
{
    if (src_pattern && dst_pattern)
    {
        clear_midi_pattern(dst_pattern);
        for (int i = 0; i < PPBAR; ++i)
            dst_pattern[i] = src_pattern[i];
    }
}

void pattern_apply_swing(midi_event *pattern, int swing_setting)
{
    midi_event new_pattern[PPBAR] = {};
    bool even16th = true;
    for (int i = 0; i < PPBAR; i += PPSIXTEENTH)
    {
        for (int j = 0; j < PPSIXTEENTH; j++)
        {
            int idx = i + j;
            if (pattern[idx].event_type)
            {
                if (even16th)
                {
                    // clean copy
                    new_pattern[idx] = pattern[idx];
                }
                else
                {
                    int new_idx =
                        idx + swing_setting * 19; // TODO magic number 19 midi
                                                  // ticks per 4% swing
                    while (new_idx < 0)
                        new_idx = PPBAR - new_idx;
                    while (new_idx >= PPBAR)
                        new_idx = new_idx - PPBAR;
                    new_pattern[new_idx] = pattern[idx];
                }
            }
        }
        even16th = 1 - even16th;
    }
    pattern_replace((midi_event *)&new_pattern, pattern);
}

void pattern_check_idx(int *idx, int pattern_len)
{
    if (*idx < 0)
        *idx += pattern_len;
    else if (*idx >= pattern_len)
        *idx -= pattern_len;
}

void pattern_apply_values(value_generator *vg, midi_event *pattern)
{
    for (int i = 0; i < PPBAR; i++)
    {
        if (pattern[i].event_type)
        {
            list_value_holder cur_val = vg->generate(vg);

            if (vg->values_type == LIST_VALUE_FLOAT_TYPE)
                pattern[i].data2 = cur_val.val;
            else
                pattern[i].data1 = get_midi_note_from_string(cur_val.wurd);
        }
    }
}

std::string PatternPrint(
    std::array<std::vector<std::shared_ptr<MusicalEvent>>, PPBAR> &events)
{
    std::stringstream ss;
    ss << "PatternEvents: ";
    for (int i = 0; i < PPBAR; i++)
    {
        if (events[i].size() > 0)
            ss << i << " ";
    }
    return ss.str();
}
