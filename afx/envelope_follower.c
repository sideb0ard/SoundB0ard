#include <math.h>

#include "../defjams.h"
#include "envelope_follower.h"

void envelope_follower_init(envelope_follower *ef, double attack_time, double release_time)
{
    ef->envelope = 0.;
    ef->attack_ms = pow( 0.01, 1.0 / (attack_time * SAMPLE_RATE * 0.001));
    ef->release_ms = pow( 0.01, 1.0 / (release_time * SAMPLE_RATE * 0.001));
}

void envelope_follower_process(envelope_follower *ef, double *src)
{
    double v = fabs(*src);
    if (v > ef->envelope)
        ef->envelope = ef->attack_ms * (ef->envelope - v) + v;
    else
        ef->envelope = ef->release_ms * (ef->envelope - v) + v;
}
