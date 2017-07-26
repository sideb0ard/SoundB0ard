#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "basicfilterpass.h"
#include "defjams.h"
#include "filter_moogladder.h"

const char *filtertype_to_name[] = {"LPF1", "HPF1", "LPF2", "HPF2", "BPF2",
                                    "BSF2", "LPF4", "HPF4", "BPF4"};

filterpass *new_filterpass()
{
    filterpass *fp = calloc(1, sizeof(filterpass));
    fp->m_fx.type = BASICFILTER;
    fp->m_fx.enabled = true;
    fp->m_fx.status = &filterpass_status;
    fp->m_fx.process = &filterpass_process_audio;

    filter_moog_init(&fp->m_filter);
    fp->m_filter.f.m_aux_control = 0.0;

    return fp;
}

void filterpass_status(void *self, char *status_string)
{
    filterpass *fp = (filterpass *)self;
    snprintf(status_string, MAX_PS_STRING_SZ,
             "MOOGLADDER! freq:%.2f q:%.2f type:%s",
             fp->m_filter.f.m_fc_control, fp->m_filter.f.m_q_control,
             filtertype_to_name[fp->m_filter.f.m_filter_type]);
}

double filterpass_process_audio(void *self, double input)
{
    filterpass *fp = (filterpass *)self;

    moog_update((filter *)&fp->m_filter);
    double filter_out = moog_gennext((filter *)&fp->m_filter, input);

    return filter_out;
}
