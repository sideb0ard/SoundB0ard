#include <stdlib.h>
#include <string.h>

#include <defjams.h>
#include <euclidean.h>
#include <mixer.h>
#include <pattern_generators/recursive_pattern_gen.h>
#include <pattern_parser.h>
#include <sequence_generator.h>
#include <utils.h>

extern mixer *mixr;

pattern_generator *new_recursive_pattern_gen()
{
    recursive_pattern_gen *r = calloc(1, sizeof(recursive_pattern_gen));
    if (!r)
    {
        printf("rWOOF!\n");
        return NULL;
    }
    r->pg.status = &recursive_pattern_gen_status;
    r->pg.generate = &recursive_pattern_generate;
    r->pg.event_notify = &recursive_pattern_gen_event_notify;
    r->pg.set_debug = &recursive_pattern_gen_set_debug;
    r->pg.type = RECURSIVE_PATTERN_TYPE;
    r->threshold = 70;
    r->divisor = 2;
    r->multi = 0.7;
    r->midi_note = 23;
    return (pattern_generator *)r;
}

void recursive_pattern_gen_status(void *self, wchar_t *wstring)
{
    recursive_pattern_gen *r = (recursive_pattern_gen *)self;
    swprintf(wstring, MAX_STATIC_STRING_SZ,
             L"[" WANSI_COLOR_WHITE
             "RECURSIVE PATTERN GEN ] - " WCOOL_COLOR_PINK
             "thresh:%.2f divisor:%.2f multi:%.2f midi_note:%d\n",
             r->threshold, r->divisor, r->multi, r->midi_note);
}

void recurse_pattern(recursive_pattern_gen *rpg, midi_event *midi_pattern,
                     int start_idx, int end_idx, int midi_note, float amp)
{
    if (start_idx < 0 || end_idx >= PPBAR)
        return;

    midi_event ev = new_midi_event(MIDI_ON, midi_note, amp);
    pattern_add_event(midi_pattern, start_idx, ev);
    if (amp > rpg->threshold)
    {
        recurse_pattern(rpg, midi_pattern, start_idx + (PPQN * rpg->multi),
                        end_idx, midi_note, amp * rpg->multi);
        recurse_pattern(rpg, midi_pattern, start_idx - (PPQN * rpg->multi),
                        end_idx, midi_note, amp * rpg->multi);
    }
}

void recursive_pattern_generate(void *self, midi_event *midi_pattern)
{
    recursive_pattern_gen *rpg = self;
    memset(midi_pattern, 0, sizeof(midi_event) * PPBAR);
    int start_idx = PPBAR / rpg->divisor;
    recurse_pattern(rpg, midi_pattern, start_idx, PPBAR - 1, rpg->midi_note,
                    128);
}

void recursive_pattern_gen_set_debug(void *self, bool b) {}
void recursive_pattern_gen_event_notify(void *self, unsigned int event_type) {}

void recursive_pattern_gen_set_thresh(recursive_pattern_gen *rpg, float thresh)
{
    rpg->threshold = thresh;
}

void recursive_pattern_gen_set_divisor(recursive_pattern_gen *rpg,
                                       float divisor)
{
    rpg->divisor = divisor;
}

void recursive_pattern_gen_set_midi_note(recursive_pattern_gen *rpg, int val)
{
    if (val >= 0 && val <= 128)
        rpg->midi_note = val;
}

void recursive_pattern_gen_set_multi(recursive_pattern_gen *rpg, float multi)
{
    rpg->multi = multi;
}
