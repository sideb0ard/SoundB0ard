#include <stdio.h>

#include "drumr_utils.h"


void build_euclidean_pattern_int(int level, int *bitmap_int, int *bitmap_position, int *count, int *remainderrr)
{
    if (level == -1) {
        *bitmap_int = *bitmap_int << 1;
    }
    else if (level == -2) {
        *bitmap_int = (*bitmap_int << 1) + 1;
    }
    else {
        for ( int i = 0; i < count[level]; i++ )
            build_euclidean_pattern_int(level - 1, bitmap_int, bitmap_position, count, remainderrr);
        if (remainderrr[level] != 0)
            build_euclidean_pattern_int(level - 2, bitmap_int, bitmap_position, count, remainderrr);
    }
}


int create_euclidean_rhythm (int num_beats, int len_pattern)
// https://ics-web.sns.ornl.gov/timing/Rep-Rate%20Tech%20Note.pdf
{
    if ( num_beats > len_pattern) {
        printf("Are ye nuts, man?!\n");
        return 0; 
    }

    // The 'remainder'
    // array is used to tell us if the level l string contains a level l − 2 string. 
    int remainderrr[len_pattern];
    // The 'count'
    // array tells us how many level l −1 strings make up a level l string. 
    int count[len_pattern];

    for (int i = 0; i < len_pattern; i++)
    { // is this necessary? 
        remainderrr[i] = 0;
        count[i] = 0;
    }

    // this is the real work, like magick
    int divisor = len_pattern - num_beats;
    remainderrr[0] = num_beats;
    int level = 0;
    do {
        count[level] = divisor / remainderrr[level];
        remainderrr[level+1] = divisor % remainderrr[level];
        divisor = remainderrr[level];
        level++;
    } while (remainderrr[level] > 1);
    count[level] = divisor;

    // now calculate return value
    int bitmap_position = 0;
    int bitmap_int = 0;

    build_euclidean_pattern_int(level, &bitmap_int, &bitmap_position, count, remainderrr);

    return bitmap_int;

}

int shift_bits_to_leftmost_position(int num, int num_of_bits_to_align_with)
{
    int first_position = 0;
    for ( int i = num_of_bits_to_align_with; i >= 0; i-- )
    {
        if ( num & (1 << i))  {
            first_position = i;
            break;
        }

    }
    int bitshift_by = num_of_bits_to_align_with - (first_position+1);
    int ret_num = num << bitshift_by;
    //print_binary_version_of_int(num);
    printf("BITSHUFING %d by %d = %d\n", num, bitshift_by, ret_num);
    //print_binary_version_of_int(ret_num);

    return ret_num;
}

void char_binary_version_of_int(int num, char bin_num[17])
{
    for ( int i = 15; i >= 0; i--) {
        if ( num & 1 << i )
            bin_num[15-i] = '1';
        else
            bin_num[15-i] = '0';
    }
    bin_num[16] = '\0';
}
