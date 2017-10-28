#include "delayapf.h"
#include "../defjams.h"

void delay_apf_init(delay_apf *d, int delay_len)
{
    delay_init(&d->m_delay, delay_len);
    d->m_apf_g = 0;
}

void delay_apf_set_apf_g(delay_apf *d, double g) { d->m_apf_g = g; }

bool delay_apf_process_audio(delay_apf *d, double *in, double *out)
{
    double w_n_d = delay_read_delay(&d->m_delay);
    double w_n = *in + d->m_apf_g * w_n_d;
    double y_n = -d->m_apf_g * w_n + w_n_d;

    delay_write_delay_and_inc(&d->m_delay, w_n);
    *out = y_n;
    return true;
}
