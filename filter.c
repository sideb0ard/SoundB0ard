#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "filter.h"
#include "utils.h"

FILTER *new_filter()
{
    FILTER *filter = (FILTER *)calloc(1, sizeof(FILTER));

    filter->m_fc_control = FILTER_FC_DEFAULT;
    filter->m_q_control = 1.0; // Q is 1 to 10
    filter->m_aux_control = 0.0;
    filter->m_saturation = 1.0;
    // filter->m_type = FILTER_TYPE_DEFAULT;  // gets set in concrete class
    filter->m_nlp = OFF;

    filter->m_fc = FILTER_FC_DEFAULT;
    filter->m_q = FILTER_Q_DEFAULT;
    filter->m_fc_mod = 0.0;

    filter->global_modmatrix = NULL;
    filter->m_mod_source_fc = DEST_NONE;
    filter->m_mod_source_fc_control = DEST_NONE;

    return filter;
}

void filter_adj_fc_control(void *filter, int direction)
{
    FILTER *self = filter;
    double val = self->m_fc_control;
    if (direction == UP)
        val += 100;
    else
        val -= 100;
    filter_set_fc_control(self, val);
}

void filter_set_fc_control(void *filter, double val)
{
    if (val > FILTER_FC_MIN && val < FILTER_FC_MAX) {
        FILTER *self = filter;
        self->m_fc_control = val;
    }
}

void filter_set_fc_mod(void *filter, double val)
{
    FILTER *self = filter;
    self->m_fc_mod = val;
}

void filter_set_q_control(void *filter, double val)
{
    FILTER *self = filter;
    self->m_q_control = val;
}

void filter_update(void *filter)
{
    FILTER *self = filter;
    filter_set_q_control(self, self->m_q_control);
    // printf("ORIG:FC: %f\n", self->m_fc);
    self->m_fc = self->m_fc_control * pitch_shift_multiplier(self->m_fc_mod);
    if (self->m_fc > FILTER_FC_MAX)
        self->m_fc = FILTER_FC_MAX;
    if (self->m_fc < FILTER_FC_MIN)
        self->m_fc = FILTER_FC_MIN;
    // printf("ADJ:FC: %f\n", self->m_fc);
}
