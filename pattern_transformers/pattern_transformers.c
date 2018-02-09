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
    parceled_pattern out_pattern = {0};
    out_pattern.len = in_pattern.len;
    // printf("\nLEFT SHIFT!\n");
    places = places * PPSIXTEENTH;
    for (int i = 0; i < in_pattern.len; i++)
    {
        if (in_pattern.pattern[i])
        {
            // printf("BIT ON AT %d\n", i);
            int target_idx = 0;
            if (direction == RIGHT)
                target_idx = i + places;
            else if (direction == LEFT)
                target_idx = i - places;

            pattern_check_idx(&target_idx, in_pattern.len);

            out_pattern.pattern[target_idx] = 1;
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
    parceled_pattern out_pattern = {0};
    out_pattern.len = in_pattern.len;
    for (int i = 0; i < in_pattern.len; i++)
    {
        if (in_pattern.pattern[i])
        {
            int target_idx = i / 2;
            pattern_check_idx(&target_idx, in_pattern.len);
            out_pattern.pattern[target_idx] = 1;
        }
    }
    out_pattern = right_shift(out_pattern, 4);
    return out_pattern;
}
