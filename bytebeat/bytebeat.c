#include "bytebeat.h"
#include "mixer.h"
#include "utils.h"
#include <limits.h>

extern mixer *mixr;

void bytebeat_init(bytebeat *b) { b->pattern_num = 0; }

double bytebeat_gennext(bytebeat *b)
{
    int t = mixr->timing_info.cur_sample;
    char bitpart = 0;
    switch (b->pattern_num)
    {
    case 0:
        bitpart = t * ((t >> 9 | t >> 13) & 25 & t >> 6);
        break;
    case 1:
        bitpart = (t >> 7 | t | t >> 6) * 10 + 4 * ((t & (t >> 13)) | t >> 6);
        break;
    case 2:
        bitpart = (t * (t >> 5 | t >> 8)) >> (t >> 16);
        break;
    case 3:
        bitpart = (t * (t >> 3 | t >> 4)) >> (t >> 7);
        break;
    case 4:
    default:
        bitpart = (t * (t >> 13 | t >> 4)) >> (t >> 3);
    }

    double scaled = scaleybum(CHAR_MIN, CHAR_MAX, -1.0, 1, bitpart);
    // printf("ORIG VAL:%d (min:%d max:%d) // SCALED: %f\n", bitpart, CHAR_MIN,
    // CHAR_MAX, scaled);

    if (scaled > 1 || scaled < -1)
        printf("BYTEBITTEN IN THE BUTT! OVER > 1!! %f (orig - %d)\n", scaled,
               bitpart);
    return scaled;
}

void bytebeat_status(bytebeat *b, wchar_t *status_string)
{
    swprintf(status_string, MAX_PS_STRING_SZ, L"Bytebeatrrr! pattern_num:%d",
             b->pattern_num);
}
