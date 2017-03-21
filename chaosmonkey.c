#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chaosmonkey.h"
#include "cmdloop.h"
#include "mixer.h"
#include "obliquestrategies.h"
#include "utils.h"

extern mixer *mixr;

chaosmonkey *new_chaosmonkey()
{
    chaosmonkey *cm = (chaosmonkey *)calloc(1, sizeof(chaosmonkey));

    printf("New chaosmonkey!\n");

    cm->sound_generator.gennext = &chaosmonkey_gen_next;
    cm->sound_generator.status = &chaosmonkey_status;
    cm->sound_generator.setvol = &chaosmonkey_setvol;
    cm->sound_generator.type = CHAOSMONKEY_TYPE;

    cm->frequency_of_wakeup = 10;
    cm->chance_of_interruption = 70;

    cm->make_suggestion = true;
    cm->take_action = true;

    return cm;
}

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
    if (link_get_sample_time(mixr->m_ableton_link) %
            (SAMPLE_RATE * cm->frequency_of_wakeup) ==
        0) {
        if (cm->chance_of_interruption > (rand() % 100)) {
            if (cm->make_suggestion && cm->take_action) {
                if ((rand() % 100) > 50) // do one or other
                {
                    oblique_strategy();
                    print_prompt();
                }
                else
                    chaosmonkey_throw_chaos();
            }
            else if (cm->make_suggestion) {
                oblique_strategy();
                print_prompt();
            }
            else if (cm->take_action)
                chaosmonkey_throw_chaos();
        }
    }
    return 0;
}

void chaosmonkey_status(void *self, wchar_t *status_string)
{
    chaosmonkey *cm = (chaosmonkey *)self;
    swprintf(status_string, MAX_PS_STRING_SZ,
             L"[chaos_monkey] wakeup: %d (sec) %d pct. Suggest: %d, Action: %d",
             cm->frequency_of_wakeup, cm->chance_of_interruption,
             cm->make_suggestion, cm->take_action);
}

void chaosmonkey_setvol(void *self, double v)
{
    (void)self;
    (void)v;
    // no-op
}

void chaosmonkey_throw_chaos()
{
    if (mixr->soundgen_num > 1) // chaos monkey is counted as one
    {
        for (int i = 0; i < mixr->soundgen_num; i++) {
            if (mixr->sound_generators[i]->type == SYNTH_TYPE) {
                int num_notes = 0;
                int midi_num[10] = {0};
                int tick[10] = {0};

                minisynth *ms = (minisynth *)mixr->sound_generators[i];
                midi_event **melody = minisynth_get_midi_loop(ms);
                for (int j = 0; j < PPNS && num_notes < 10; j++) {
                    if (melody[j] != NULL) {
                        if (melody[j]->event_type == 144) {
                            midi_num[num_notes] = melody[j]->data1;
                            tick[num_notes] = melody[j]->tick;
                            num_notes++;
                        }
                    }
                }
                int num_new_notes = rand() % 4;
                for (int k = 0; k < num_new_notes; k++) {
                    int newnum = (rand() % 2) > 0 ? 4 : 3;
                    int randy = rand() % 3;
                    switch (randy) {
                    case 0:
                        break; // same octave
                    case 1:
                        newnum += 12; // up an octave
                    case 2:
                        newnum -= 12;
                    }

                    int new_midi_num = (midi_num[k] + newnum) % 128;

                    int note_on_tick = (tick[k] + PPNS / 2) % PPNS;
                    int note_off_tick = note_on_tick + (PPS * 4);
                    if (note_off_tick >= PPNS)
                        note_off_tick = PPNS - 1;

                    midi_event *ev = new_midi_event(note_on_tick, 144, // noteon
                                                    new_midi_num, 127);
                    ev->delete_after_use = true;

                    midi_event *ev2 =
                        new_midi_event(note_off_tick, 128, // noteoff
                                       new_midi_num, 127);
                    ev2->delete_after_use = true;

                    minisynth_add_event(ms, ev);
                    minisynth_add_event(ms, ev2);
                }
            }
        }
    }
}
