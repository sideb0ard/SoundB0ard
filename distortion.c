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
    snprintf(status_string, MAX_PS_STRING_SZ, "Distortion! threshold:%.2f",
             d->m_threshold);
}

double distortion_process(void *self, double input)
{
    distortion *d = (distortion *)self;
    double returnval = 0;
    if (input >= 0)
        returnval = fmin(input, d->m_threshold);
    else
        returnval = fmax(input, -d->m_threshold);
    returnval /= d->m_threshold;
    return returnval;
}
