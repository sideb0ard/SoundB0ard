#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <value_generator.h>

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

void value_generator_generate(void *self, void *return_data)
{
    value_generator *vg = (value_generator *)self;
    return_data = &vg->values[vg->cur_idx];

    if (++(vg->cur_idx) >= vg->values_len)
        vg->cur_idx = 0;
}

static char *s_value_types[] = {"CHAR*", "FLOAT"};

void value_generator_status(void *self, wchar_t *wstring)
{
    value_generator *vg = (value_generator *)self;
    char *list_vals = calloc(sizeof(char),  20*SIZE_OF_WURD);
    int num_vals_to_show = vg->values_len < 19 ? vg->values_len : 19;

    if (vg->values_type == VALUE_LIST_CHAR_TYPE)
    {
        for (int i = 0; i < num_vals_to_show; i++)
        {
            char **values = vg->values;
            char *wurd = values[i];
            printf("WURD %s\n", wurd);
            strcat(list_vals, wurd);
            if (i != num_vals_to_show -1)
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
            if (i != num_vals_to_show -1)
                strcat(list_vals, " ");
        }

    }

    swprintf(wstring, MAX_STATIC_STRING_SZ,
             L"[" WANSI_COLOR_WHITE "VALUE GENERATOR]  - " WCOOL_COLOR_PINK
             "type:%s len:(%d) vals:%s",
             s_value_types[vg->values_type], vg->values_len, list_vals);

   free(list_vals); 
}
