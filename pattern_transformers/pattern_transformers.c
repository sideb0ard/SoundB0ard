#include <stdio.h>

#include <pattern_parser.h>
#include <pattern_transformers.h>

static void pattern_check_idx(int *idx, int pattern_len)
{
    if (*idx < 0)
        *idx += pattern_len;
    else if (*idx >= pattern_len)
        *idx -= pattern_len;
}

static void _shift(midi_event *in_pattern, int places, int direction)
{
    midi_event out_pattern[PPBAR] = {};
    // printf("\nLEFT SHIFT!\n");
    places = places * PPSIXTEENTH;
    for (int i = 0; i < PPBAR; i++)
    {
        if (in_pattern[i].event_type)
        {
            // printf("BIT ON AT %d\n", i);
            int target_idx = 0;
            if (direction == RIGHT)
                target_idx = i + places;
            else if (direction == LEFT)
                target_idx = i - places;

            pattern_check_idx(&target_idx, PPBAR);

            out_pattern[target_idx] = in_pattern[i];
        }
    }
    clear_pattern(in_pattern);
    for (int i = 0; i < PPBAR; i++)
        in_pattern[i] = out_pattern[i];
}

void left_shift(midi_event *in_pattern, int places)
{
    return _shift(in_pattern, places, LEFT);
}

void right_shift(midi_event *in_pattern, int places)
{
    return _shift(in_pattern, places, RIGHT);
}

void brak(midi_event *in_pattern)
{
    midi_event out_pattern[PPBAR] = {};
    for (int i = 0; i < PPBAR; i++)
    {
        if (in_pattern[i].event_type)
        {
            int target_idx = i / 2;
            pattern_check_idx(&target_idx, PPBAR);
            out_pattern[target_idx] = in_pattern[i];
        }
    }
    right_shift(out_pattern, 4);
    clear_pattern(in_pattern);
    for (int i = 0; i < PPBAR; i++)
        in_pattern[i] = out_pattern[i];
}
