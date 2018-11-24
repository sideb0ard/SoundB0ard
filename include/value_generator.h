#include <defjams.h>

enum
{
    VALUE_LIST_CHAR_TYPE,
    VALUE_LIST_FLOAT_TYPE,
};

typedef struct value_generator
{
    int values_len;
    unsigned int values_type;
    void *values;
    int cur_idx; // circular idx
    void (*generate)(void *self, void *return_data);
    void (*status)(void *self, wchar_t *wstring);
} value_generator;

value_generator *new_value_generator(unsigned int values_type, int values_len, void *values);
void value_generator_generate(void *self, void *return_data);
void value_generator_status(void *self, wchar_t *wstring);

