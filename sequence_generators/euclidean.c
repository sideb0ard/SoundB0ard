#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "euclidean.h"
#include "mixer.h"
#include "sequencer_utils.h"

extern mixer *mixr;
sequence_generator *new_euclidean(int num_hits, int num_steps)
{
    euclidean *e = calloc(1, sizeof(euclidean));
    if (!e)
    {
        printf("WOOF!\n");
        return NULL;
    }
    e->sg.status = &euclidean_status;
    e->sg.generate = &euclidean_generate;

    e->num_hits = num_hits;
    e->num_steps = num_steps;

    printf("NEW EUCLID - numhits:%d steps:%d\n", e->num_hits, e->num_steps);

    return (sequence_generator *)e;
}

void euclidean_status(void *self, wchar_t *wstring)
{
    euclidean *e = (euclidean *)self;
    swprintf(wstring, MAX_PS_STRING_SZ,
             L"[" WANSI_COLOR_WHITE "SEQUENCE GEN ] - " WCOOL_COLOR_PINK
             "wee EUCLIDEAN hits:%d steps:%d",
             e->num_hits, e->num_steps);
}

void build_euclidean_pattern_int(int level, int *bitmap_int,
                                 int *bitmap_position, int *count,
                                 int *remainderrr)
{
    if (level == -1)
    {
        *bitmap_int = *bitmap_int << 1;
    }
    else if (level == -2)
    {
        *bitmap_int = (*bitmap_int << 1) + 1;
    }
    else
    {
        for (int i = 0; i < count[level]; i++)
        {
            build_euclidean_pattern_int(level - 1, bitmap_int, bitmap_position,
                                        count, remainderrr);
        }
        if (remainderrr[level] != 0)
        {
            build_euclidean_pattern_int(level - 2, bitmap_int, bitmap_position,
                                        count, remainderrr);
        }
    }
}

void build_euclidean_pattern_string(int level, char *bitmap_string, int *count,
                                    int *remaindrrr)
{
    if (level == -1)
    {
        strcat(bitmap_string, "0");
    }
    else if (level == -2)
    {
        strcat(bitmap_string, "1");
    }
    else
    {
        for (int i = 0; i < count[level]; i++)
        {
            build_euclidean_pattern_string(level - 1, bitmap_string, count,
                                           remaindrrr);
        }
        if (remaindrrr[level] != 0)
        {
            build_euclidean_pattern_string(level - 2, bitmap_string, count,
                                           remaindrrr);
        }
    }
}

// https://ics-web.sns.ornl.gov/timing/Rep-Rate%20Tech%20Note.pdf
int euclidean_generate(void *self, void *data)
{
    euclidean *e = (euclidean *)self;
    printf("GENERATING!\nnum_hits:%d num_steps:%d\n", e->num_hits,
           e->num_steps);

    return create_euclidean_rhythm(e->num_hits, e->num_steps);
}

int create_euclidean_rhythm(int num_hits, int num_steps)
{
    int remaindrrr[num_steps];
    int count[num_steps];
    memset(count, 0, num_steps * sizeof(int));
    memset(remaindrrr, 0, num_steps * sizeof(int));

    int level = 0;
    int divisor = num_steps - num_hits;
    remaindrrr[level] = num_hits;
    do
    {
        count[level] = divisor / remaindrrr[level];
        // printf("LEVEL:%d DIVISOR:%d REMAINDERRR:%d COUNT:%d\n", level,
        // divisor, remaindrrr[level], count[level]);
        remaindrrr[level + 1] = divisor % remaindrrr[level];
        divisor = remaindrrr[level];
        level++;
    } while (remaindrrr[level] > 1);
    count[level] = divisor;

    // now calculate return value
    int bitmap_position = 0;
    int bitmap_int = 0;

    char bitmap_string[num_steps + 1];
    memset(bitmap_string, 0, (num_steps + 1) * sizeof(char));
    build_euclidean_pattern_string(level, bitmap_string, count, remaindrrr);
    printf("STRINGPATTERN: %s\n", bitmap_string);
    build_euclidean_pattern_int(level, &bitmap_int, &bitmap_position, count,
                                remaindrrr);
    printf("PATTERN int: %d\n", bitmap_int);
    char bin_ver_num[17];
    char_binary_version_of_int(bitmap_int, bin_ver_num);
    printf("PATTERN: %s\n", bin_ver_num);
    return bitmap_int;
}
