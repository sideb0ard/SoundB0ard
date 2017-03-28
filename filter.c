#include <stdio.h>
#include <stdlib.h>

#include "defjams.h"
#include "filter.h"
#include "utils.h"

void filter_setup(filter *f)
{
    f->m_q_control = 1.0; // Q is 1 to 10
    f->m_fc = FILTER_FC_DEFAULT;
    f->m_q = FILTER_Q_DEFAULT;
    f->m_fc_control = FILTER_FC_DEFAULT;

    f->m_fc_mod = 0.0;
    f->m_aux_control = 0.0;
    f->m_nlp = OFF;
    f->m_saturation = 1.0;

    f->m_v_modmatrix = NULL;

    f->m_mod_source_fc = DEST_NONE;
    f->m_mod_source_fc_control = DEST_NONE;

    //f->m_global_filter_params = NULL;
}

// void filter_set_fc_control(filter *f, double val)
//{
//    if (val > FILTER_FC_MIN && val < FILTER_FC_MAX) {
//        f->m_fc_control = val;
//    }
//}

void filter_set_fc_mod(filter *f, double val) { f->m_fc_mod = val; }

//void filter_set_q_control(filter *f, double val) { f->m_q_control = val; }

void filter_update(filter *f)
{
    // printf("1ORIG:FC: %f\n", f->m_fc);
    // printf("2MY FC_CONTROL: %f\n", f->m_fc_control);
    // printf("GLOBAL FC_CONTROL IS %f\n",
    // f->m_global_filter_params->fc_control);
    if (f->m_global_filter_params) {
        f->m_aux_control = f->m_global_filter_params->aux_control;
        f->m_fc_control = f->m_global_filter_params->fc_control;
        f->m_q_control = f->m_global_filter_params->q_control;
        f->m_saturation = f->m_global_filter_params->saturation;
        f->m_filter_type = f->m_global_filter_params->filter_type;
        f->m_nlp = f->m_global_filter_params->nlp;
    }

    // printf("2RIG:FC: %f\n", f->m_fc);
    //printf("Q!!! : %f\n", f->m_q_control);
    // exit(0);

    if (f->m_v_modmatrix) {
        f->m_fc_mod = f->m_v_modmatrix->m_destinations[f->m_mod_source_fc];
        if (f->m_v_modmatrix->m_destinations[f->m_mod_source_fc_control] > 0)
            f->m_fc_control =
                f->m_v_modmatrix->m_destinations[f->m_mod_source_fc_control];
    }

    //f->set_q_control(f, f->m_q_control);

    f->m_fc = f->m_fc_control * pitch_shift_multiplier(f->m_fc_mod);
    // printf("OOFT, M_FC %f\n", f->m_fc);

    if (f->m_fc > FILTER_FC_MAX)
        f->m_fc = FILTER_FC_MAX;
    if (f->m_fc < FILTER_FC_MIN)
        f->m_fc = FILTER_FC_MIN;
    //printf("NOW:_Q_: %f\n", f->m_q_control);
    //printf("NOW:FC: %f\n", f->m_fc);
    //printf("NOW MY FC_CONTROL: %f\n", f->m_fc_control);
}

void filter_reset(filter *f)
{
    (void)f; // noop
}

void filter_init_global_parameters(filter *self, global_filter_params *params)
{
    self->m_global_filter_params = params;
    self->m_global_filter_params->aux_control = self->m_aux_control;
    self->m_global_filter_params->fc_control = self->m_fc_control;
    self->m_global_filter_params->q_control = self->m_q_control;
    self->m_global_filter_params->saturation = self->m_saturation;
    self->m_global_filter_params->filter_type = self->m_filter_type;
    self->m_global_filter_params->nlp = self->m_nlp;
}
