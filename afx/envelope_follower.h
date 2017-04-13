#pragma once

typedef struct envelope_follower
{
    double envelope;
    double attack_ms; // milliseconds
    double release_ms;
} envelope_follower;

void envelope_follower_init(envelope_follower *ef, double attack_time, double release_time);
void envelope_follower_process(envelope_follower *ef, double *src);
