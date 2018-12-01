#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <value_generator.h>
#include <wchar.h>

value_generator *new_value_generator(unsigned int values_type, int values_len,
                                     void *values)
{
    value_generator *vg = calloc(1, sizeof(value_generator));
    if (!vg)
    {
        printf("RETCH!\n");
        return NULL;
    }
    vg->values_type = values_type;
    vg->values_len = values_len;
    vg->values = values;
    vg->cur_idx = 0;

    vg->generate = value_generator_generate;
    vg->status = value_generator_status;

    return vg;
}

list_value_holder value_generator_generate(void *self)
{
    value_generator *vg = (value_generator *)self;
    list_value_holder return_val = {.value_type = vg->values_type};

    if (vg->values_type == LIST_VALUE_CHAR_TYPE)
    {
        char **values = vg->values;
        char *wurd = values[vg->cur_idx];
        strcpy(return_val.wurd, wurd);
    }
    else
    {
        float *values = vg->values;
        return_val.val = values[vg->cur_idx];
    }

    if (++(vg->cur_idx) >= vg->values_len)
        vg->cur_idx = 0;

    return return_val;
}

static char *s_value_types[] = {"CHAR*", "FLOAT"};

void value_generator_status(void *self, wchar_t *wstring)
{
    value_generator *vg = (value_generator *)self;
    char *list_vals = calloc(sizeof(char), 20 * SIZE_OF_WURD);
    int num_vals_to_show = vg->values_len < 19 ? vg->values_len : 19;

    if (vg->values_type == LIST_VALUE_CHAR_TYPE)
    {
        for (int i = 0; i < num_vals_to_show; i++)
        {
            char **values = vg->values;
            char *wurd = values[i];
            printf("WURD %s\n", wurd);
            strcat(list_vals, wurd);
            if (i != num_vals_to_show - 1)
                strcat(list_vals, " ");
        }
    }
    else
    {
        for (int i = 0; i < num_vals_to_show; i++)
        {
            char scratch[33] = {};
            float *values = vg->values;
            sprintf(scratch, "%.2f", values[i]);
            strcat(list_vals, scratch);
            if (i != num_vals_to_show - 1)
                strcat(list_vals, " ");
        }
    }

    swprintf(wstring, MAX_STATIC_STRING_SZ,
             L"[" WANSI_COLOR_WHITE "VALUE GENERATOR]  - " WCOOL_COLOR_PINK
             "type:%s len:(%d) vals:%s",
             s_value_types[vg->values_type], vg->values_len, list_vals);

    free(list_vals);
}
