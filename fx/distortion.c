#include <stdlib.h>

#include "distortion.h"
#include "math.h"
#include "mixer.h"

extern mixer *mixr;

distortion *new_distortion(void)
{
    distortion *d = calloc(1, sizeof(distortion));
    d->m_fx.type = DISTORTION;
    d->m_fx.enabled = true;
    d->m_fx.process = &distortion_process;
    d->m_fx.status = &distortion_status;
    d->m_fx.event_notify = &fx_noop_event_notify;

    d->m_threshold = 0.707;
    return d;
}

void distortion_set_threshold(distortion *d, double val)
{
    if (val >= 0.01 && val <= 1.0)
        d->m_threshold = val;
    else
        printf("Val must be between 0.01 and 1\n");
}

void distortion_status(void *self, char *status_string)
{
    distortion *d = (distortion *)self;
    snprintf(status_string, MAX_STATIC_STRING_SZ, "Distortion! threshold:%.2f",
             d->m_threshold);
}

stereo_val distortion_process(void *self, stereo_val input)
{
    stereo_val out = {};
    distortion *d = (distortion *)self;

    if (input.left >= 0)
        out.left = fmin(input.left, d->m_threshold);
    else
        out.left = fmax(input.left, -d->m_threshold);
    out.left /= d->m_threshold;

    if (input.right >= 0)
        out.right = fmin(input.right, d->m_threshold);
    else
        out.right = fmax(input.right, -d->m_threshold);
    out.right /= d->m_threshold;

    return out;

}
