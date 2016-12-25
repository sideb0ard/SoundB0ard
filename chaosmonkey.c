#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chaosmonkey.h"
#include "cmdloop.h"
#include "mixer.h"
#include "utils.h"

extern mixer *mixr;

chaosmonkey *new_chaosmonkey()
{
    chaosmonkey *cm = calloc(1, sizeof(chaosmonkey));

    printf("New chaosmonkey!\n");

    cm->sound_generator.gennext = &chaosmonkey_gen_next;
    cm->sound_generator.status = &chaosmonkey_status;
    cm->sound_generator.setvol = &chaosmonkey_setvol;
    cm->sound_generator.type = CHAOSMONKEY_TYPE;

    cm->frequency_of_wakeup = 5;
    cm->chance_of_interruption = 50;

    cm->make_suggestion = true;
    cm->take_action = true;

    return cm;
}

// TODO - pretty sure these are all race conditions
void chaosmonkey_change_wakeup_freq(chaosmonkey *cm, int num_seconds)
{
    cm->frequency_of_wakeup = num_seconds;
}

void chaosmonkey_change_chance_interrupt(chaosmonkey *cm, int percent)
{
    cm->chance_of_interruption = percent;
}

void chaosmonkey_suggest_mode(chaosmonkey *cm, bool val)
{
    cm->make_suggestion = val;
}

void chaosmonkey_action_mode(chaosmonkey *cm, bool val)
{
    cm->take_action = val;
}

double chaosmonkey_gen_next(void *self)
{
    chaosmonkey *cm = (chaosmonkey *)self;
    if (mixr->cur_sample % (SAMPLE_RATE * cm->frequency_of_wakeup) == 0) {
        if (cm->chance_of_interruption > (rand() % 100)) {
            printf("I awake!\n");
        }
    }
    return 0;
}

void chaosmonkey_status(void *self, char *status_string)
{
    chaosmonkey *cm = (chaosmonkey *)self;
    snprintf(status_string, 119,
             "[chaos_monkey] wakeup: %d (sec) %d pct. Suggest: %d, Action: %d",
             cm->frequency_of_wakeup, cm->chance_of_interruption,
             cm->make_suggestion, cm->take_action);
}

void chaosmonkey_setvol(void *self, double v)
{
    (void)self;
    (void)v;
    // no-op
}
