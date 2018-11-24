#pragma once

#include <defjams.h>

enum
{
    LIST_VALUE_CHAR_TYPE,
    LIST_VALUE_FLOAT_TYPE,
};

typedef struct list_value_holder
{
    unsigned int value_type;
    char wurd[SIZE_OF_WURD];
    float val;
} list_value_holder;

typedef struct value_generator
{
    int values_len;
    unsigned int values_type;
    void *values;
    int cur_idx; // circular idx
    list_value_holder (*generate)(void *self);
    void (*status)(void *self, wchar_t *wstring);
} value_generator;

value_generator *new_value_generator(unsigned int values_type, int values_len, void *values);
list_value_holder value_generator_generate(void *self);
void value_generator_status(void *self, wchar_t *wstring);

