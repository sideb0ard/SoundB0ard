#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "euclidean.h"
#include "mixer.h"
#include "pattern_utils.h"
#include "utils.h"

extern mixer *mixr;

constexpr char const *s_euclid_mode[] = {"STATIC", "UP", "DOWN", "RANDOM"};

pattern_generator *new_euclidean(int num_hits, int num_steps)
{
    euclidean *e = (euclidean *)calloc(1, sizeof(euclidean));
    if (!e)
    {
        printf("WOOF!\n");
        return NULL;
    }
    e->sg.status = &euclidean_status;
    e->sg.generate = &euclidean_generate;
    e->sg.event_notify = &euclidean_event_notify;
    e->sg.debug = &euclidean_set_debug;
    e->sg.type = EUCLIDEAN;

    e->num_hits = num_hits;
    e->num_steps = num_steps;

    e->actual_num_hits = num_hits;
    e->actual_num_steps = num_steps;

    e->mode = EUCLID_RANDOM;

    printf("NEW EUCLID - numhits:%d steps:%d\n", e->num_hits, e->num_steps);

    return (pattern_generator *)e;
}

void euclidean_status(void *self, wchar_t *wstring)
{
    euclidean *e = (euclidean *)self;
    swprintf(wstring, MAX_STATIC_STRING_SZ,
             L"[" WANSI_COLOR_WHITE "EUCLIDEAN GEN ] - " WCOOL_COLOR_PINK
             "mode:%s hits:%d steps:%d actual_hits:%d actual_steps:%d",
             s_euclid_mode[e->mode], e->num_hits, e->num_steps,
             e->actual_num_hits, e->actual_num_steps);
}

static void build_euclidean_pattern_short(int level, uint16_t *bitmap_int,
                                          uint16_t *bitmap_len, int *count,
                                          int *remainderrr)
{
    if (level == -1)
    {
        (*bitmap_len)++;
        *bitmap_int = *bitmap_int << 1;
    }
    else if (level == -2)
    {
        (*bitmap_len)++;
        *bitmap_int = (*bitmap_int << 1) + 1;
    }
    else
    {
        for (int i = 0; i < count[level]; i++)
        {
            build_euclidean_pattern_short(level - 1, bitmap_int, bitmap_len,
                                          count, remainderrr);
        }
        if (remainderrr[level] != 0)
        {
            build_euclidean_pattern_short(level - 2, bitmap_int, bitmap_len,
                                          count, remainderrr);
        }
    }
}

// static void build_euclidean_pattern_string(int level, char *bitmap_string,
//                                           int *count, int *remaindrrr)
//{
//    if (level == -1)
//    {
//        strcat(bitmap_string, "0");
//    }
//    else if (level == -2)
//    {
//        strcat(bitmap_string, "1");
//    }
//    else
//    {
//        for (int i = 0; i < count[level]; i++)
//        {
//            build_euclidean_pattern_string(level - 1, bitmap_string, count,
//                                           remaindrrr);
//        }
//        if (remaindrrr[level] != 0)
//        {
//            build_euclidean_pattern_string(level - 2, bitmap_string, count,
//                                           remaindrrr);
//        }
//    }
//}

// https://ics-web.sns.ornl.gov/timing/Rep-Rate%20Tech%20Note.pdf
void euclidean_generate(void *self, void *data)
{
    euclidean *e = (euclidean *)self;
    midi_event *pattern = (midi_event *)data;
    uint16_t bit_pattern = create_euclidean_rhythm(e->actual_num_hits, 16);
    apply_short_to_midi_pattern(bit_pattern, pattern);
}

uint16_t create_euclidean_rhythm(int num_hits, int num_steps)
{
    if (num_hits == 0 || num_steps == 0)
        return 0;

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
    uint16_t bitmap_len = 0;
    uint16_t bitmap_int = 0;

    // char bitmap_string[num_steps + 1];
    // memset(bitmap_string, 0, (num_steps + 1) * sizeof(char));
    // build_euclidean_pattern_string(level, bitmap_string, count, remaindrrr);
    // printf("STRINGPATTERN: %s\n", bitmap_string);

    build_euclidean_pattern_short(level, &bitmap_int, &bitmap_len, count,
                                  remaindrrr);
    //// printf("PATTERN int: %d Len:%d\n", bitmap_int, bitmap_len);
    // char bin_ver_num[17];
    // char_binary_version_of_short(bitmap_int, bin_ver_num);
    // printf("PATTERN: %s\n", bin_ver_num);
    // print_bin_num(bitmap_int);

    uint16_t max_bits_to_align_with = 16; // max

    // find first set bit
    uint16_t first_bit = 0;
    for (int i = max_bits_to_align_with; i >= 0; i--)
    {
        if (bitmap_int & (1 << i))
        {
            first_bit = i;
            // printf("FIRST BIT IS %d\n", i);
            break;
        }
    }

    uint16_t bitshift_by = (max_bits_to_align_with - 1) - first_bit;
    // printf("SHIfting left by %d places\n", bitshift_by);
    uint16_t aligned_bitmap = bitmap_int << bitshift_by;
    // printf("ALLIGNED\n");
    // print_bin_num(aligned_bitmap);
    return aligned_bitmap;
}

void euclidean_event_notify(void *self, broadcast_event event)
{
    euclidean *e = (euclidean *)self;
    switch (event.type)
    {
    case (TIME_QUARTER_TICK):
        if (e->mode == EUCLID_UP)
        {
            e->actual_num_hits++;
            if (e->actual_num_hits > e->num_hits)
                e->actual_num_hits = 1;
        }
        else if (e->mode == EUCLID_DOWN)
        {
            e->actual_num_hits--;
            if (e->actual_num_hits == 0)
                e->actual_num_hits = e->num_hits;
        }
        else if (e->mode == EUCLID_RANDOM)
        {
            int dice = rand() % 5;
            switch (dice)
            {
            case (0):
                e->actual_num_hits = 3;
                break;
            case (1):
                e->actual_num_hits = 4;
                break;
            case (2):
                e->actual_num_hits = 5;
                break;
            case (3):
                e->actual_num_hits = 7;
            case (4):
            default:
                e->actual_num_hits = 9;
            }
        }
        break;
    }
}

void euclidean_change_hits(euclidean *e, int num_hits)
{
    e->num_hits = num_hits;
    e->actual_num_hits = num_hits;
}

void euclidean_change_steps(euclidean *e, int num_steps)
{
    e->num_steps = num_steps;
    e->actual_num_steps = num_steps;
}

void euclidean_change_mode(euclidean *e, unsigned int mode)
{
    if (mode < EUCLID_NUM_MODES)
        e->mode = mode;
}
void euclidean_set_debug(void *self, bool b)
{
    euclidean *e = (euclidean *)self;
    e->sg.debug = b;
}
