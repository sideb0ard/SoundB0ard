#include <stdio.h>

#include <pattern_parser.h>
#include <pattern_transformers.h>
#include <pattern_utils.h>
#include <stdlib.h>

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
    clear_midi_pattern(in_pattern);
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
    midi_event scratch_pattern[PPBAR] = {};
    for (int i = 0; i < PPBAR; i++)
    {
        if (in_pattern[i].event_type)
        {
            int target_idx = i / 2;
            pattern_check_idx(&target_idx, PPBAR);
            scratch_pattern[target_idx] = in_pattern[i];
            scratch_pattern[target_idx].data2 /= 1.5;
        }
    }
    right_shift(scratch_pattern, 8);
    for (int i = 0; i < PPBAR; i++)
        if (scratch_pattern[i].event_type)
            midi_pattern_add_event(in_pattern, i, scratch_pattern[i]);
}

void echo(midi_event *in_pattern)
{
    midi_event scratch_pattern[PPBAR] = {};
    for (int i = 0; i < PPBAR; i++)
    {
        if (in_pattern[i].event_type)
        {
            if (in_pattern[i].data2 > 30)
            {
                int target_idx = i + (PPQN / 2);
                pattern_check_idx(&target_idx, PPBAR);
                scratch_pattern[target_idx] = in_pattern[i];
                scratch_pattern[target_idx].data2 /= 2.5;
                scratch_pattern[target_idx].delete_after_use = true;
            }
        }
    }
    for (int i = 0; i < PPBAR; i++)
        if (scratch_pattern[i].event_type)
            midi_pattern_add_event(in_pattern, i, scratch_pattern[i]);
}

void dense(midi_event *in_pattern, int density)
{
    if (density < 2)
        return;

    int density_fraction = PPBAR / density;
    midi_event *scratch_pattern = calloc(sizeof(midi_event), density_fraction);
    for (int i = 0; i < PPBAR; i++)
    {
        if (in_pattern[i].event_type)
        {
            int target_idx = i / density;
            pattern_check_idx(&target_idx, PPBAR);
            scratch_pattern[target_idx] = in_pattern[i];
            scratch_pattern[target_idx].data2 /= 2.5;
            scratch_pattern[target_idx].delete_after_use = true;
        }
    }

    for (int i = 0; i < PPBAR / density; i++)
    {
        if (scratch_pattern[i].event_type)
        {
            for (int j = 0; j < density; j++)
            {
                midi_pattern_add_event(in_pattern, j * density_fraction,
                                       scratch_pattern[i]);
            }
        }
    }

    free(scratch_pattern);
}

void reverse(midi_event *in_pattern)
{
    //printf("REVERSE!\n");
    midi_event scratch_pattern[PPBAR] = {};
    for (int i = 0; i < PPBAR; i++)
    {
        if (in_pattern[i].event_type)
        {
            int target_idx = PPBAR - i - 1;
            scratch_pattern[target_idx] = in_pattern[i];
        }
    }
    clear_midi_pattern(in_pattern);
    for (int i = 0; i < PPBAR; i++)
        if (scratch_pattern[i].event_type)
            midi_pattern_add_event(in_pattern, i, scratch_pattern[i]);
}
