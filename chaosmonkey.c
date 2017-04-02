#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

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

    cm->make_suggestion = false;
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
    if (mixr->start_of_loop) {
        // DO SOMETHING
        printf("TICK: %d PPQN: %d\n", mixr->tick, PPQN);
        int randy1 = rand() % 4; 
        for (int i = 0; i < randy1; i++)
        {
            int t = mixr->cur_sample;
		    int bzzt = 0;
		    int randy = rand() % 5;
		    switch(randy) {
		    case 0:
                bzzt = t * ((t >> 9 | t >> 13) & 25 & t >> 6);
            case 1:
                bzzt = (t >> 7 | t | t >> 6) * 10 + 4 * ((t & (t >> 13)) | t >> 6);
            case 2:
                bzzt = (t * (t >> 5 | t >> 8)) >> (t >> 16);
            case 3:
                bzzt = (t * (t >> 3 | t >> 4)) >> (t >> 7);
            case 4:
                 bzzt = (t * (t >> 13 | t >> 4)) >> (t >> 3);
            }

            int nom = scaleybum(INT_MIN, INT_MAX, 30, 100, bzzt);

            if (mixr->debug_mode)
                printf("BZZZT %d Midi Num: %d\n", bzzt, nom);

            chaosmonkey_throw_chaos(nom);
        }
    }
    if (cm->last_midi_tick != mixr->tick)
    {
        //printf("MIDI Tick!\n");
        cm->last_midi_tick = mixr->tick;
    }
    if (cm->last_sixteenth != mixr->sixteenth_note_tick)
    {
        //printf("16th Tick!\n");
        cm->last_sixteenth = mixr->sixteenth_note_tick;

		
    }

    //if (mixr->cur_sample % (SAMPLE_RATE * cm->frequency_of_wakeup) == 0) {
    //    if (cm->chance_of_interruption > (rand() % 100)) {
    //        if (cm->make_suggestion && cm->take_action) {
    //            if ((rand() % 100) > 50) // do one or other
    //            {
    //                oblique_strategy();
    //                print_prompt();
    //            }
    //            else
    //                chaosmonkey_throw_chaos();
    //        }
    //        else if (cm->make_suggestion) {
    //            oblique_strategy();
    //            print_prompt();
    //        }
    //        else if (cm->take_action)
    //            chaosmonkey_throw_chaos();
    //    }
    //}
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

void chaosmonkey_throw_chaos(int note)
{
    if (mixr->soundgen_num > 1) // chaos monkey is counted as one
    {
        for (int i = 0; i < mixr->soundgen_num; i++) {
            if (mixr->sound_generators[i]->type == SYNTH_TYPE) {
                //int num_notes = 0;
                //int midi_num[10] = {0};
                //int tick[10] = {0};

                minisynth *ms = (minisynth *)mixr->sound_generators[i];
                //minisynth_handle_midi_note(ms, note, 126);

                int rand_timing = rand() % 16;
                int note_on_tick = (mixr->tick % PPNS) + rand_timing * PPQN;
                int note_off_tick = note_on_tick + 3 * PPQN;
                midi_event *on_event = new_midi_event(
                    note_on_tick, 144, note, 126);
                on_event->delete_after_use = true;
                midi_event *off_event = new_midi_event(
                    note_off_tick, 128, note, 126);
                off_event->delete_after_use = true;

                minisynth_add_event(ms, on_event);
                minisynth_add_event(ms, off_event);

                return;
                //midi_event **melody = minisynth_get_midi_loop(ms);
                //for (int j = 0; j < PPNS && num_notes < 10; j++) {
                //    if (melody[j] != NULL) {
                //        if (melody[j]->event_type == 144) {
                //            midi_num[num_notes] = melody[j]->data1;
                //            tick[num_notes] = melody[j]->tick;
                //            num_notes++;
                //        }
                //    }
                //}
                //int num_new_notes = rand() % 4;
                //for (int k = 0; k < num_new_notes; k++) {
                //    int newnum = (rand() % 2) > 0 ? 4 : 3;
                //    int randy = rand() % 3;
                //    switch (randy) {
                //    case 0:
                //        break; // same octave
                //    case 1:
                //        newnum += 12; // up an octave
                //    case 2:
                //        newnum -= 12;
                //    }

                //    int new_midi_num = (midi_num[k] + newnum) % 128;

                //    int note_on_tick = (tick[k] + PPNS / 2) % PPNS;
                //    int note_off_tick = note_on_tick + (PPS * 4);
                //    if (note_off_tick >= PPNS)
                //        note_off_tick = PPNS - 1;

                //    midi_event *ev = new_midi_event(note_on_tick, 144, // noteon
                //                                    new_midi_num, 127);
                //    ev->delete_after_use = true;

                //    midi_event *ev2 =
                //        new_midi_event(note_off_tick, 128, // noteoff
                //                       new_midi_num, 127);
                //    ev2->delete_after_use = true;

                //    minisynth_add_event(ms, ev);
                //    minisynth_add_event(ms, ev2);
                //}
            }
        }
    }
}
