#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "defjams.h"
#include "filter.h"
#include "utils.h"

Filter::Filter()
{
    m_fc = FILTER_FC_DEFAULT;
    m_q = FILTER_Q_DEFAULT;
    m_fc_control = FILTER_FC_DEFAULT;
    m_q_control = 1.0; // Q is 1 to 10

    m_fc_mod = 0.0;
    m_aux_control = 0.0;
    m_nlp = 0;
    m_saturation = 1.0;

    m_mod_source_fc = DEST_NONE;
    m_mod_source_fc_control = DEST_NONE;

    global_filter_params = NULL;
}

void Filter::SetFcControl(double val)
{
    if (val >= FILTER_FC_MIN && val <= FILTER_FC_MAX)
    {
        m_fc_control = val;
        std::cout << "SET FCCCCC!:" << val << std::endl;
        Update();
    }
    else
    {
        printf("FC must be between %d and %d\n", FILTER_FC_MIN, FILTER_FC_MAX);
    }
}

void Filter::SetFcMod(double val) { m_fc_mod = val; }
void Filter::SetType(unsigned int type)
{
    if (type < NUM_FILTER_TYPES)
    {
        m_filter_type = type;
        Update();
    }
    else
        printf("Type must be between 0 and %d\n", NUM_FILTER_TYPES - 1);
}

void Filter::SetQControl(double val)
{
    // do nothing. Needs override
}

void Filter::Update()
{
    if (global_filter_params)
    {
        m_aux_control = global_filter_params->aux_control;
        m_fc_control = global_filter_params->fc_control;
        m_q_control = global_filter_params->q_control;
        m_saturation = global_filter_params->saturation;
        m_filter_type = global_filter_params->filter_type;
        m_nlp = global_filter_params->nlp;
    }

    if (modmatrix)
    {
        std::cout << "moDMATRX!\n";
        m_fc_mod = modmatrix->destinations[m_mod_source_fc];
        std::cout << "FC MOD, YO" << m_fc_mod << std::endl;
        if (modmatrix->destinations[m_mod_source_fc_control] > 0)
        {
            m_fc_control = modmatrix->destinations[m_mod_source_fc_control];
            std::cout << "FC CONTROL, YO" << m_fc_mod << std::endl;
        }
    }

    SetQControl(m_q_control);

    m_fc = m_fc_control * pitch_shift_multiplier(m_fc_mod);

    if (m_fc > FILTER_FC_MAX)
    {
        std::cout << "OOh, capping it at MAX\n";
        m_fc = FILTER_FC_MAX;
    }
    if (m_fc < FILTER_FC_MIN)
    {
        std::cout << "OOh, capping it at MIN\n";
        m_fc = FILTER_FC_MIN;
    }
}

void Filter::Reset() {}

void Filter::InitGlobalParameters(GlobalFilterParams *params)
{
    global_filter_params = params;
    global_filter_params->aux_control = m_aux_control;
    global_filter_params->fc_control = m_fc_control;
    global_filter_params->q_control = m_q_control;
    global_filter_params->saturation = m_saturation;
    global_filter_params->filter_type = m_filter_type;
    global_filter_params->nlp = m_nlp;
}
