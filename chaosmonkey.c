#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chaosmonkey.h"
#include "cmdloop.h"
#include "defjams.h"
#include "mixer.h"
#include "obliquestrategies.h"
#include "synthbase.h"
#include "utils.h"

extern mixer *mixr;

chaosmonkey *new_chaosmonkey(int soundgen)
{
    if (!mixer_is_valid_soundgen_num(mixr, soundgen))
    {
        return NULL;
    }
    chaosmonkey *cm = (chaosmonkey *)calloc(1, sizeof(chaosmonkey));

    printf("New chaosmonkey!\n");

    osc_new_settings((oscillator *)&cm->m_lfo);
    lfo_set_soundgenerator_interface(&cm->m_lfo);
    lfo_start_oscillator((oscillator *)&cm->m_lfo);

    cm->sound_generator.gennext = &chaosmonkey_gen_next;
    cm->sound_generator.status = &chaosmonkey_status;
    cm->sound_generator.setvol = &chaosmonkey_setvol;
    cm->sound_generator.type = CHAOSMONKEY_TYPE;

    cm->frequency_of_wakeup = 10;
    cm->chance_of_interruption = 70;

    cm->make_suggestion = false;
    cm->take_action = true;

    cm->soundgen = soundgen;
    cm->soundgen_type = mixr->sound_generators[soundgen]->type;

    // TODO - for the moment only minisynth
    minisynth *ms = (minisynth *)mixr->sound_generators[soundgen];
    minisynth_set_sustain_override(ms, true);
    minisynth_midi_note_on(ms, 60, 100);

    cm->chaos_mode = rand() % 2;
    printf("CHAOS MODE! %d\n", cm->chaos_mode);

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

stereo_val chaosmonkey_gen_next(void *self, mixer_timing_info timing_info)
{
    chaosmonkey *cm = (chaosmonkey *)self;
    (void)cm;
    (void)timing_info;
    // osc_update(&cm->m_lfo.osc);
    // double unused_quad = 0.0;
    // double lfo_out = lfo_do_oscillate(&cm->m_lfo.osc, &unused_quad);
    // minisynth *ms = (minisynth *)mixr->sound_generators[cm->soundgen];
    // minisynth_set_filter_mod(ms, lfo_out);
    // switch (cm->chaos_mode) {
    // case (0):
    //     printf("CHAOS MODE ZERO\n");
    //     break;
    // case (1):
    //     printf("CHAOS MODE ONE\n");
    //     break;
    // }

    if (mixr->timing_info.is_sixteenth)
    {
    }
    //    if (mixr->soundgen_num > 1) // chaos monkey is counted as one
    //    {
    //        for (int i = 0; i < mixr->soundgen_num; i++) {
    //            if (mixr->sound_generators[i]->type == SYNTH_TYPE) {
    //                if ((rand()%100) > 90) {
    //                    minisynth *ms = (minisynth
    //                    *)mixr->sound_generators[i];
    //                    int randy1 = rand() % 4;
    //                    for (int i = 0; i < randy1; i++) {
    //                        int midi_num = 0;
    //                        int rand_note = rand() % 3;
    //                        switch (rand_note) {
    //                        case(0):
    //                            midi_num = ms->m_last_midi_note;
    //                            break;
    //                        case(1):
    //                            midi_num = ms->m_last_midi_note + 4;
    //                            break;
    //                        case(2):
    //                            midi_num = ms->m_last_midi_note + 7;
    //                            break;
    //                        }
    //                        if (midi_num > 7)
    //                            minisynth_handle_midi_note(ms, midi_num, 100,
    //                            false);
    //                    }
    //                }
    //            }
    //        }
    //    }
    //}

    return (stereo_val){0, 0};
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

void chaosmonkey_add_note_at_random_time(minisynth *ms, int note)
{
    int rand_timing = rand() % 16;
    int note_on_tick =
        (mixr->timing_info.midi_tick + rand_timing * PPQN) % PPNS;
    int note_off_tick = (note_on_tick + 3 * PPQN + 7) % PPNS;
    midi_event on_event = new_midi_event(note_on_tick, 144, note, 126);
    on_event.delete_after_use = true;
    midi_event off_event = new_midi_event(note_off_tick, 128, note, 126);
    off_event.delete_after_use = true;

    synthbase_add_event(&ms->base, ms->base.cur_melody, on_event);

    return;
}
