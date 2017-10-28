#pragma once
#include <stdbool.h>
#include "delay.h"

typedef struct delay_apf
{
    delay m_delay; // base class
    double m_apf_g; // one co-efficient
} delay_apf;

void delay_apf_init(delay_apf *d, int delay_len);
void delay_apf_set_apf_g(delay_apf *d, double g);
bool delay_apf_process_audio(delay_apf *d, double *in, double *out);

