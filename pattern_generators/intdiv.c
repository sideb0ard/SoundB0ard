#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "intdiv.h"
#include "mixer.h"
#include "pattern_generator.h"
#include "pattern_utils.h"
extern mixer *mixr;

static char *s_intdiv_status_names[] = {"STATIC", "RAND"};

pattern_generator *new_intdiv(int num_parts, char *allowed_parts)
{
    intdiv *id = calloc(1, sizeof(intdiv));
    if (!id)
    {
        printf("WOOF!\n");
        return NULL;
    }
    id->num_parts = num_parts;

    intdiv_set_allowed_part_sizes(id, allowed_parts);

    id->sg.status = &intdiv_status;
    id->sg.generate = &intdiv_generate;
    id->sg.event_notify = &intdiv_event_notify;
    id->sg.set_debug = &intdiv_set_debug;
    id->sg.type = INTDIV;
    return (pattern_generator *)id;
}

void intdiv_status(void *self, wchar_t *wstring)
{
    intdiv *id = (intdiv *)self;
    swprintf(
        wstring, MAX_STATIC_STRING_SZ,
        L"[" WANSI_COLOR_WHITE "INTDIV PATTERN GEN ] - " WCOOL_COLOR_PINK
        "selected:%d mode:%s(%d) num_pats:%d num_notes:%d allowed_parts[%s]",
        id->selected_pattern, s_intdiv_status_names[id->mode], id->mode, id->num_patterns,
        id->num_parts, id->allowed_part_sizes_as_string);
}

static bool _is_allowed(intdiv *id, int part_size)
{
    for (int i = 0; i < id->num_allowed_part_sizes; i++)
        if (part_size == id->allowed_part_sizes[i])
            return true;
    return false;
}

static void _intdiv_add_to_patterns(intdiv *id)
{
    char num[17] = {0};
    for (int i = 0; i < id->num_parts; i++)
    {
        strcat(num, "1");
        int num_zeros = id->parts[i] - 1;
        for (int j = 0; j < num_zeros; j++)
            strcat(num, "0");
    }
    uint16_t binnum = char_to_short(num);
    if (id->num_patterns < MAX_ALLOWED_PATTERNS)
        id->patterns[id->num_patterns++] = binnum;
    else
        printf("Nah, too many patterns!\n");
}

// this is modified from http://abrazol.com/books/rhythm1/prog/compam.c
static void _intdiv_generate_bit_patterns(intdiv *id, int cur_pulse,
                                          int cur_part_size, int part_num)
{
    if (cur_pulse == 0)
    {
        if (part_num == id->num_parts - 1 && _is_allowed(id, cur_part_size))
        {
            id->parts[part_num] = cur_part_size;
            _intdiv_add_to_patterns(id);
        }
        return;
    }

    if (part_num < id->num_parts - 1 && _is_allowed(id, cur_part_size))
    {
        id->parts[part_num] = cur_part_size;
        _intdiv_generate_bit_patterns(id, cur_pulse - 1, 1, part_num + 1);
    }
    _intdiv_generate_bit_patterns(id, cur_pulse - 1, cur_part_size + 1,
                                  part_num);
}

void intdiv_generate(void *self, void *data)
{
    intdiv *id = (intdiv *)self;
    midi_event *midi_pattern = (midi_event *)data;

    int pattern_num = id->selected_pattern;
    if (id->mode == INTDIV_RANDOM)
        pattern_num = rand() % id->num_patterns;
    uint16_t bit_pattern = id->patterns[pattern_num];
    apply_short_to_midi_pattern(bit_pattern, midi_pattern);
}
void intdiv_set_debug(void *self, bool b) {}

void intdiv_event_notify(void *self, unsigned int event_type) {}

void intdiv_set_allowed_part_sizes(intdiv *id, char *allowed_parts)
{
    strncpy(id->allowed_part_sizes_as_string, allowed_parts, 127);

    int allowed_string_sz = strlen(allowed_parts);
    char *stripped_string = calloc(allowed_string_sz - 1, sizeof(char));
    strncpy(stripped_string, allowed_parts + 1, allowed_string_sz - 2);

    char const *sep = " ";
    char *tok, *last_tok;
    id->num_allowed_part_sizes = 0;
    for (tok = strtok_r(stripped_string, sep, &last_tok); tok;
         tok = strtok_r(NULL, sep, &last_tok))
    {
        id->allowed_part_sizes[id->num_allowed_part_sizes++] = atoi(tok);
    }

    for (int i = 0; i < id->num_allowed_part_sizes; i++)
        printf("GOt %d\n", id->allowed_part_sizes[i]);

    _intdiv_generate_bit_patterns(id, 15, 1, 0);
}

void intdiv_print_patterns(intdiv *id)
{
    char num[17] = {0};
    for (int i = 0; i < id->num_patterns; i++)
    {
        short_to_char(id->patterns[i], num);
        printf("[%d] %s\n", i, num);
    }
}

void intdiv_set_selected_pattern(intdiv *id, unsigned int pat_num)
{
    if (pat_num < id->num_patterns)
        id->selected_pattern = pat_num;
}

void intdiv_set_mode(intdiv *id, unsigned int mode)
{
    if (mode < INTDIV_NUM_MODES)
        id->mode = mode;
}
