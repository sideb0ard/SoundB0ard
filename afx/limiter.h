#pragma once
// cribbed from http://www.musicdsp.org/showone.php?id=265

#include "envelope_follower.h"

typedef struct limiter {

    envelope_follower e;

} limiter;

void limiter_init(limiter *l, double attack_time, double release_time);
double limiter_process(limiter *l, double *in);
