#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "bitshift.h"
#include "defjams.h"
#include "euclidean.h"
#include "mixer.h"
#include "sequencer_utils.h"
#include "synthbase.h"
#include "utils.h"
#include <pattern_parser.h>

extern const int key_midi_mapping[NUM_KEYS];
extern const char *key_names[NUM_KEYS];
extern const compat_key_list compat_keys[NUM_KEYS];

extern mixer *mixr;

void synthbase_init(synthbase *base, void *parent,
                    unsigned int parent_synth_type)
{
    base->parent = parent;
    base->parent_synth_type = parent_synth_type;

    base->num_patterns = 1;
    base->max_generation = 0;

    base->multi_pattern_mode = true;
    base->cur_pattern_iteration = 1;
    for (int i = 0; i < MAX_NUM_MIDI_LOOPS; i++)
        base->pattern_multiloop_count[i] = 1;

    base->sample_rate = 44100;
    base->sample_rate_counter = 0;

    base->generate_src = -99;
    base->last_midi_note = 23;
    base->midi_note = 23;
    base->sustain_note_ms = 200;
    base->note_mode = false;
}

void synthbase_generate_pattern(synthbase *base, int gen_src, bool keep_note, bool save_pattern)
{
    if (mixer_is_valid_seq_gen_num(mixr, gen_src))
    {
        if (save_pattern)
        {
            synthbase_set_backup_mode(base, true);
            base->restore_pending = true;
        }

        synthbase_clear_pattern_ready_for_new_one(base, base->cur_pattern);

        synthbase_stop(base);
        sequence_generator *sg = mixr->sequence_generators[gen_src];
        uint16_t bits = sg->generate(sg, NULL);

        int patternlen = 16;
        for (int i = 0; i < patternlen; i++)
        {
            int shift_by = patternlen - 1 - i;
            if (bits & (1 << shift_by))
            {
                synthbase_add_note(base, base->cur_pattern, i, base->midi_note,
                                   keep_note);
            }
        }
    }
}

void synthbase_set_sample_rate(synthbase *base, int sample_rate)
{
    // does sample and hold to sample down
    printf("Chh-ch-changing SAMPLE_RATE!: %d\n", sample_rate);
    base->sample_rate = sample_rate;
    base->sample_rate_ratio = SAMPLE_RATE / (double)sample_rate;
    base->sample_rate_counter = 0;
}

void synthbase_set_multi_pattern_mode(synthbase *ms, bool pattern_mode)
{
    ms->multi_pattern_mode = pattern_mode;
    ms->cur_pattern_iteration = ms->pattern_multiloop_count[ms->cur_pattern];
}

void synthbase_set_pattern_loop_num(synthbase *self, int pattern_num,
                                    int loop_num)
{
    self->pattern_multiloop_count[pattern_num] = loop_num;
}

int synthbase_add_pattern(synthbase *ms) { return ms->num_patterns++; }

void synthbase_dupe_pattern(midi_pattern *from, midi_pattern *to)
{
    for (int i = 0; i < PPBAR; i++)
        (*to)[i] = (*from)[i];
}

void synthbase_switch_pattern(synthbase *ms, unsigned int pattern_num)
{
    if (pattern_num < (unsigned)ms->num_patterns)
        ms->cur_pattern = pattern_num;
}

void synthbase_reset_pattern_all(synthbase *ms)
{
    for (int i = 0; i < MAX_NUM_MIDI_LOOPS; i++)
    {
        synthbase_reset_pattern(ms, i);
    }
}

void synthbase_stop(synthbase *base)
{
    if (base->parent_synth_type == MINISYNTH_TYPE)
    {
        minisynth *p = (minisynth *)base->parent;
        minisynth_stop(p);
    }
    else if (base->parent_synth_type == DXSYNTH_TYPE)
    {
        dxsynth *p = (dxsynth *)base->parent;
        dxsynth_stop(p);
    }
}

void synthbase_reset_pattern(synthbase *base, unsigned int pattern_num)
{
    synthbase_stop(base);

    if (pattern_num < MAX_NUM_MIDI_LOOPS)
    {
        memset(base->patterns[pattern_num], 0, sizeof(midi_pattern));
    }
}

// sound generator interface //////////////
void synthbase_status(synthbase *base, wchar_t *status_string)
{
    wchar_t scratch[256] = {0};
    wchar_t patternstr[33] = {0};

    swprintf(scratch, 255, L"\nnote_mode:%d chord_mode:%d", base->note_mode,
             base->chord_mode);
    wcscat(status_string, scratch);
    memset(scratch, 0, 256);
    for (int i = 0; i < base->num_patterns; i++)
    {
        pattern_to_string(base->patterns[i], patternstr);
        swprintf(scratch, 255, L"\n[%d]  %ls  numloops: %d", i, patternstr,
                 base->pattern_multiloop_count[i]);
        wcscat(status_string, scratch);
    }
    wcscat(status_string, WANSI_COLOR_RESET);
}

void synthbase_event_notify(void *self, unsigned int event_type)
{
    soundgenerator *parent = (soundgenerator *)self;
    synthbase *base = get_synthbase(parent);
    int idx;

    switch (event_type)
    {
    case (TIME_START_OF_LOOP_TICK):
        if (base->restore_pending)
        {
            synthbase_dupe_pattern(&base->backup_pattern_while_getting_crazy,
                                   &base->patterns[base->cur_pattern]);
            base->restore_pending = false;
        }
        else if (base->multi_pattern_mode && base->num_patterns > 1)
        {
            // synthbase_stop(base);
            base->cur_pattern_iteration--;
            if (base->cur_pattern_iteration <= 0)
            {
                if (base->parent_synth_type == MINISYNTH_TYPE)
                    minisynth_midi_note_off((minisynth *)parent, 0, 0,
                                            true /* all notes off */);
                else if (base->parent_synth_type == DXSYNTH_TYPE)
                    dxsynth_midi_note_off((dxsynth *)parent, 0, 0,
                                          true /* all notes off */);

                int next_pattern = (base->cur_pattern + 1) % base->num_patterns;

                base->cur_pattern = next_pattern;
                base->cur_pattern_iteration =
                    base->pattern_multiloop_count[base->cur_pattern];
            }
        }
        break;
    case (TIME_MIDI_TICK):
        idx = mixr->timing_info.midi_tick % PPBAR;
        if (base->patterns[base->cur_pattern][idx].event_type)
        {
            midi_parse_midi_event(parent,
                                  &base->patterns[base->cur_pattern][idx]);
        }

        break;
    }
}

void synthbase_add_event(synthbase *base, int pattern_num, int midi_tick,
                         midi_event ev)
{
    int target_tick = midi_tick;
    while (base->patterns[pattern_num][target_tick].event_type)
    {
        if (mixr->debug_mode)
            printf("Gotsz a tick already - bump!\n");
        target_tick++;
        if (target_tick == PPBAR) // wrap around
            target_tick = 0;
    }
    base->patterns[pattern_num][target_tick] = ev;
}

void synthbase_clear_pattern_ready_for_new_one(synthbase *ms, int pattern_num)
{
    memset(ms->patterns[pattern_num], 0, sizeof(midi_pattern));
}

void synthbase_nudge_pattern(synthbase *ms, int pattern_num, int sixteenth)
{
    if (sixteenth >= 16)
        sixteenth = sixteenth % 16;

    int sixteenth_of_loop = PPBAR / 16.0;

    midi_event *source_pattern = synthbase_get_pattern(ms, pattern_num);

    midi_pattern new_loop;
    for (int i = 0; i < PPBAR; i++)
    {
        if (source_pattern[i].event_type)
        {
            int new_tick = (i + (sixteenth * sixteenth_of_loop)) % PPBAR;
            new_loop[new_tick] = source_pattern[i];
        }
    }

    synthbase_set_pattern(ms, pattern_num, new_loop);
}

bool is_valid_pattern_num(synthbase *ms, int pattern_num)
{
    if (pattern_num >= 0 && pattern_num < MAX_NUM_MIDI_LOOPS)
    {
        if (pattern_num >= ms->num_patterns)
            ms->num_patterns = pattern_num + 1;
        return true;
    }
    return false;
}

void synthbase_set_note_mode(synthbase *base, bool b) { base->note_mode = b; }

void synthbase_set_chord_mode(synthbase *base, bool b) { base->chord_mode = b; }

void synthbase_set_backup_mode(synthbase *base, bool b)
{
    if (b)
    {
        synthbase_dupe_pattern(&base->patterns[0],
                               &base->backup_pattern_while_getting_crazy);
        // base->m_settings_backup_while_getting_crazy = base->m_settings;
        base->multi_pattern_mode = false;
        base->cur_pattern = 0;
    }
    else
    {
        synthbase_dupe_pattern(&base->backup_pattern_while_getting_crazy,
                               &base->patterns[0]);
        // base->m_settings = base->m_settings_backup_while_getting_crazy;
        base->multi_pattern_mode = true;
    }
}

int synthbase_get_num_notes(synthbase *ms)
{
    int notecount = 0;
    for (int i = 0; i < ms->num_patterns; i++)
        for (int j = 0; j < PPBAR; j++)
            if (ms->patterns[i][j].event_type == 144)
                notecount++;
    return notecount;
}

int synthbase_get_notes_from_pattern(midi_pattern loop,
                                     int return_midi_notes[10])
{
    int idx = 0;
    for (int i = 0; i < PPBAR; i++)
    {
        midi_event ev = loop[i];
        if (ev.event_type == MIDI_ON)
        { // note on
            if (!is_int_member_in_array(ev.data1, return_midi_notes, 10))
            {
                return_midi_notes[idx++] = ev.data1;
                if (idx == 10)
                    break;
                // return idx;
            }
        }
    }
    return idx; // num notes
}

int synthbase_get_num_patterns(void *self)
{
    synthbase *ms = (synthbase *)self;
    return ms->num_patterns;
}

void synthbase_set_num_patterns(void *self, int num_patterns)
{
    printf("BASS! how low can you go %d!\n", num_patterns);
    synthbase *ms = (synthbase *)self;
    if (num_patterns > 0)
    {
        ms->num_patterns = num_patterns;
    }
}

void synthbase_make_active_track(void *self, int pattern_num)
{
    synthbase *ms = (synthbase *)self;
    ms->cur_pattern =
        pattern_num; // TODO - standardize - PATTERN? TRACK? pattern?!?!
}

void synthbase_print_patterns(synthbase *ms)
{
    for (int i = 0; i < ms->num_patterns; i++)
        midi_pattern_print(ms->patterns[i]);
}

void synthbase_add_note(synthbase *ms, int pattern_num, int step, int midi_note,
                        bool keep_note)
{
    int mstep = step * PPSIXTEENTH;
    synthbase_add_micro_note(ms, pattern_num, mstep, midi_note, keep_note);
}

void synthbase_add_micro_note(synthbase *ms, int pattern_num, int mstep,
                              int midi_note, bool keep_note)
{
    if (is_valid_pattern_num(ms, pattern_num) && mstep < PPBAR)
    {
        midi_event on = new_midi_event(MIDI_ON, midi_note, 128);

        if (!keep_note)
        {
            on.delete_after_use = true;
        }

        synthbase_add_event(ms, pattern_num, mstep, on);
    }
    else
    {
        printf("Adding MICRO note - not valid pattern-num(%d) || step no "
               "good(%d)\n",
               pattern_num, mstep);
    }
}

void synthbase_rm_note(synthbase *ms, int pattern_num, int step)
{
    int mstep = step * PPSIXTEENTH;
    synthbase_rm_micro_note(ms, pattern_num, mstep);
}

void synthbase_rm_micro_note(synthbase *ms, int pat_num, int tick)
{
    if (is_valid_pattern_num(ms, pat_num) && tick < PPBAR)
    {
        memset(&ms->patterns[ms->cur_pattern][tick], 0, sizeof(midi_event));
    }
    else
    {
        printf("Not a valid pattern num: %d \n", pat_num);
    }
}

void synthbase_mv_note(synthbase *ms, int pattern_num, int fromstep, int tostep)
{
    int mfromstep = fromstep * PPSIXTEENTH;
    int mtostep = tostep * PPSIXTEENTH;
    synthbase_mv_micro_note(ms, pattern_num, mfromstep, mtostep);
}

void synthbase_mv_micro_note(synthbase *ms, int pattern_num, int fromstep,
                             int tostep)
{
    if (is_valid_pattern_num(ms, pattern_num))
    {
        ms->patterns[pattern_num][tostep] = ms->patterns[pattern_num][fromstep];
        memset(&ms->patterns[pattern_num][fromstep], 0, sizeof(midi_event));
    }
}

void synthbase_import_midi_from_file(synthbase *base, char *filename)
{
    printf("Importing MIDI from %s\n", filename);
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("Couldn't open yer file!\n");
        return;
    }

    char *item, *last_s;
    char const *sep = "::";
    char line[4096];
    while (fgets(line, sizeof(line), fp))
    {
        printf("%s", line);
        int max_tick = PPBAR * base->num_patterns;
        int count = 0;
        int tick = 0;
        int status = 0;
        int midi_note = 0;
        int midi_vel = 0;
        for (item = strtok_r(line, sep, &last_s); item;
             item = strtok_r(NULL, sep, &last_s))
        {
            switch (count)
            {
            case 0:
                tick = atoi(item);
                if (tick >= max_tick)
                {
                    printf("TICK OVER!: %d\n", tick);
                    synthbase_add_pattern(base);
                    tick = tick % PPBAR;
                }
                break;
            case 1:
                status = atoi(item);
                break;
            case 2:
                midi_note = atoi(item);
                break;
            case 3:
                midi_vel = atoi(item);
                break;
            }
            count++;
            printf("ITEM! %s\n", item);
        }
        if (count == 4)
        {
            printf("GOtzz %d %d %d %d %d\n", count, tick, status, midi_note,
                   midi_vel);
            midi_event ev = new_midi_event(status, midi_note, midi_vel);
            synthbase_add_event(base, base->cur_pattern, tick, ev);
        }
    }

    synthbase_set_multi_pattern_mode(base, true);
    fclose(fp);
}

int synthbase_change_octave_pattern(synthbase *base, int pattern_num,
                                    int direction)
{
    // printf("Changing octave of %d - direction: %s\n", pattern_num,
    //       direction == 1 ? "UP" : "DOWN");
    if (is_valid_pattern_num(base, pattern_num))
    {
        for (int i = 0; i < PPBAR; i++)
            if (base->patterns[pattern_num][i].event_type)
            {
                int new_midi_num = base->patterns[pattern_num][i].data1;
                if (direction == 1) // up
                    new_midi_num += 12;
                else
                    new_midi_num -= 12;
                if (new_midi_num >= 0 && new_midi_num < 128)
                    base->patterns[pattern_num][i].data1 = new_midi_num;
            }
    }
    else
        return 1;

    return 0;
}

void synthbase_set_sustain_note_ms(synthbase *base, int sustain_note_ms)
{
    if (sustain_note_ms > 0)
        base->sustain_note_ms = sustain_note_ms;
}

void synthbase_set_rand_key(synthbase *base)
{
    int dice = rand() % 3;
    printf("RAND KEY/NOTE! %d\n", dice);
    switch (dice)
    {
    case (0):
        base->last_midi_note = base->midi_note;
        break;
    case (1):
        base->last_midi_note = base->midi_note + 4;
        break;
    case (2):
        base->last_midi_note = base->midi_note + 7;
        break;
    }
}

void synthbase_set_midi_note(synthbase *base, int note)
{
    base->midi_note = note;
}

midi_event *synthbase_get_pattern(synthbase *base, int pattern_num)
{
    if (is_valid_pattern_num(base, pattern_num))
        return base->patterns[pattern_num];
    return NULL;
}

void synthbase_set_pattern(void *self, int pattern_num, midi_event *pattern)
{
    synthbase *base = (synthbase *)self;
    printf("SET PATTERN!\n");
    midi_pattern_print(pattern);
    printf("PATTERN NUM! %d\n", pattern_num);
    if (is_valid_pattern_num(base, pattern_num))
    {
        printf("VALUD!\n");
        clear_pattern(base->patterns[pattern_num]);
        for (int i = 0; i < PPBAR; i++)
        {
            synthbase_add_event(base, pattern_num, i, pattern[i]);
            // base->patterns[pattern_num][i] = pattern[i];
            if (base->chord_mode && pattern[i].event_type)
            {
                printf("midi note: %d\n", pattern[i].data1);
                midi_event copy = pattern[i];
                copy.data1 = pattern[i].data1 + 4;
                synthbase_add_event(base, pattern_num, i, copy);
                copy.data1 = pattern[i].data1 + 7;
                synthbase_add_event(base, pattern_num, i, copy);
            }
        }
    }
}

bool synthbase_list_presets(unsigned int synthtype)
{
    FILE *presetzzz = NULL;
    switch (synthtype)
    {
    case (MINISYNTH_TYPE):
        presetzzz = fopen(MOOG_PRESET_FILENAME, "r+");
        break;
    case (DXSYNTH_TYPE):
        presetzzz = fopen(DX_PRESET_FILENAME, "r+");
        break;
    }

    if (presetzzz == NULL)
        return false;

    char line[256];
    while (fgets(line, sizeof(line), presetzzz))
    {
        printf("%s\n", line);
    }

    fclose(presetzzz);

    return true;
}
