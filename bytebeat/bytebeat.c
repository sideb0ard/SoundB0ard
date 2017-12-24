#include <string.h>
#include <stdlib.h>

#include "sequence_generator.h"
#include "defjams.h"
#include "bytebeat_interpreter.h"
#include "utils.h"

#define LARGEST_POSSIBLE 2048
sequence_generator *new_sequence_generator(char wurds[][SIZE_OF_WURD], int num_wurds)
{
    printf("NUM WURDS %d\n", num_wurds);
    char *pattern_static[LARGEST_POSSIBLE] = {0};
    char *pattern = (char*)pattern_static;
    for (int i = 2; i < num_wurds; i++)
    {
        strncat(pattern, wurds[i], SIZE_OF_WURD);
        strcat(pattern, " ");
    }
    printf("PATTERN! is %s\n", pattern);
    //if (!isvalid_pattern(
    //char *pattern = "t * ( ( t >> 9 | t >> 13 ) & 25 & t >> 6)";
    if (strlen(pattern) > 1023)
    {
        printf("Max pattern size of 1024\n");
        return NULL;
    }
    sequence_generator *sg = calloc(1, sizeof(sequence_generator));
    if (!sg)
    {
        printf("WOOF!\n");
        return NULL;
    }
    strncpy(sg->pattern, pattern, 1023);
    sg->status = &sequence_generator_status;
    sg->generate = &sequence_generator_generate;

    sg->rpn_stack = new_rpn_stack(pattern);

    return sg;
}
void sequence_generator_change_pattern(sequence_generator *sg, char *pattern)
{
    // TODO
}

void sequence_generator_status(void *self, wchar_t *wstring)
{
    sequence_generator *sg = (sequence_generator*) self;
    swprintf(wstring, MAX_PS_STRING_SZ,
            L"[" WANSI_COLOR_WHITE "SEQUENCE GEN ] - " WCOOL_COLOR_PINK
            "pattern: %s", sg->pattern);
}

int sequence_generator_generate(void *self)
{
    sequence_generator *sg = (sequence_generator*) self;
    char ans = interpreter(sg->rpn_stack);
    print_bin_num(ans);
    return ans;
}

