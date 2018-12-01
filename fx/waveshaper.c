#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "waveshaper.h"

waveshaper *new_waveshaper()
{
    waveshaper *ws = calloc(1, sizeof(waveshaper));
    waveshaper_init(ws);

    ws->m_fx.type = WAVESHAPER;
    ws->m_fx.enabled = true;
    ws->m_fx.status = &waveshaper_status;
    ws->m_fx.process = &waveshaper_process_audio;
    ws->m_fx.event_notify = &fx_noop_event_notify;

    return ws;
}

void waveshaper_init(waveshaper *ws)
{
    ws->m_arc_tan_k_pos = 1;
    ws->m_arc_tan_k_neg = 1;
    ws->m_stages = 1;
    ws->m_invert_stages = 0; // off
}

void waveshaper_status(void *self, char *status_string)
{
    waveshaper *ws = (waveshaper *)self;
    snprintf(status_string, MAX_STATIC_STRING_SZ,
             "k_pos:%.2f k_neg:%.2f"
             "stages:%d invert:%d",
             ws->m_arc_tan_k_pos, ws->m_arc_tan_k_neg, ws->m_stages,
             ws->m_invert_stages);
}

void waveshaper_set_arc_tan_k_pos(waveshaper *d, double val)
{
    if (val >= 0.1 && val <= 20)
        d->m_arc_tan_k_pos = val;
    else
        printf("Val must be between 0.1 and 20\n");
}

void waveshaper_set_arc_tan_k_neg(waveshaper *d, double val)
{
    if (val >= 0.1 && val <= 20)
        d->m_arc_tan_k_neg = val;
    else
        printf("Val must be between 0.1 and 20\n");
}

void waveshaper_set_stages(waveshaper *d, unsigned int val)
{
    if (val > 0 && val < 11)
        d->m_stages = val;
    else
        printf("Val must be between 1 and 10\n");
}

void waveshaper_set_invert_stages(waveshaper *d, unsigned int val)
{
    if (val < 2)
        d->m_invert_stages = val;
    else
        printf("Val must be 0 or 1\n");
}

double _process_audio(waveshaper *ws, double input)
{
    double xn = input;
    for (unsigned int i = 0; i < ws->m_stages; i++)
    {
        if (xn >= 0)
            xn = (1.0 / atan(ws->m_arc_tan_k_pos)) *
                 atan(ws->m_arc_tan_k_pos * xn);
        else
            xn = (1.0 / atan(ws->m_arc_tan_k_neg)) *
                 atan(ws->m_arc_tan_k_neg * xn);
        if (ws->m_invert_stages && i % 2 == 0)
            xn *= -1.0;
    }
    return xn;
}
stereo_val waveshaper_process_audio(void *self, stereo_val input)
{
    waveshaper *ws = (waveshaper *)self;
    stereo_val out = {};
    out.left = _process_audio(ws, input.left);
    out.right = _process_audio(ws, input.right);

    return out;
}
