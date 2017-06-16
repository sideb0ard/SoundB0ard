#include <stdio.h>
#include "biquad.h"

void biquad_flush_delays(biquad *b)
{
    b->m_xz_1 = 0;
    b->m_xz_2 = 0;
    b->m_yz_1 = 0;
    b->m_yz_2 = 0;
}

double biquad_process(biquad *b, double xn)
{
    // printf("BIQUADZ: m_a0:%f, m_a1:%f, m_xz_1:%f m_a2:%f m_xz_2:%f m_b1:%f m_yz_1:%f b2:%f m_yz_2:%f\n",
    //         b->m_a0, b->m_a1, b->m_xz_1, b->m_a2, b->m_xz_2, b->m_b1, b->m_yz_1, b->m_b2, b->m_yz_2);

    double yn = b->m_a0 * xn + b->m_a1*b->m_xz_1 + b->m_a2* b->m_xz_2 - b->m_b1*b->m_yz_1 - b->m_b2*b->m_yz_2;

    if (yn > 0.0 && yn < FLT_MIN_PLUS) yn = 0; 
    if (yn < 0.0 && yn > FLT_MIN_MINUS) yn = 0; 

    b->m_yz_2 = b->m_yz_1;
    b->m_yz_1 = yn;

    b->m_xz_2 = b->m_xz_1;
    b->m_xz_1 = xn;

    return yn;
}

