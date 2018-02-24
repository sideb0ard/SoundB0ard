#include <stdio.h>

#include <pattern_transformers.h>

static void pattern_check_idx(int *idx, int pattern_len)
{
    if (*idx < 0)
        *idx += pattern_len;
    else if (*idx >= pattern_len)
        *idx -= pattern_len;
}

static parceled_pattern _shift(parceled_pattern in_pattern, int places,
                               int direction)
{
    parceled_pattern out_pattern = {};
    // printf("\nLEFT SHIFT!\n");
    places = places * PPSIXTEENTH;
    for (int i = 0; i < PPBAR; i++)
    {
        if (in_pattern.pattern[i].event_type)
        {
            // printf("BIT ON AT %d\n", i);
            int target_idx = 0;
            if (direction == RIGHT)
                target_idx = i + places;
            else if (direction == LEFT)
                target_idx = i - places;

            pattern_check_idx(&target_idx, PPBAR);

            out_pattern.pattern[target_idx] = in_pattern.pattern[i];
        }
    }
    return out_pattern;
}

parceled_pattern left_shift(parceled_pattern in_pattern, int places)
{
    return _shift(in_pattern, places, LEFT);
}

parceled_pattern right_shift(parceled_pattern in_pattern, int places)
{
    return _shift(in_pattern, places, RIGHT);
}

parceled_pattern brak(parceled_pattern in_pattern)
{
    parceled_pattern out_pattern = {};
    for (int i = 0; i < PPBAR; i++)
    {
        if (in_pattern.pattern[i].event_type)
        {
            int target_idx = i / 2;
            pattern_check_idx(&target_idx, PPBAR);
            out_pattern.pattern[target_idx] = in_pattern.pattern[i];
        }
    }
    out_pattern = right_shift(out_pattern, 4);
    return out_pattern;
}
