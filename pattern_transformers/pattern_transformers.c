#include <stdio.h>

#include <pattern_transformers.h>

parceled_pattern left_shift(parceled_pattern in_pattern, int places)
{
    parceled_pattern out_pattern = {0};
    out_pattern.len = in_pattern.len;
    printf("\nLEFT SHIFT!\n");
    places = places * PPSIXTEENTH;
    for (int i = 0; i < in_pattern.len; i++)
    {
        if (in_pattern.pattern[i])
        {
            printf("BIT ON AT %d\n", i);
            int target_idx = i - places;
            if (target_idx < 0)
            {
                printf("TARGETIDX under ZERO!:%d\n", target_idx);
                target_idx += in_pattern.len;
            }
            printf("SETTING BIT AT %d\n", target_idx);
            out_pattern.pattern[target_idx] = 1;
        }
    }
    return out_pattern;
}

parceled_pattern right_shift(parceled_pattern in_pattern, int places);
