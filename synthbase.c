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
extern const wchar_t *sparkchars;

void synthbase_init(synthbase *base, void *parent,
                    unsigned int parent_synth_type)
{
    base->parent = parent;
    base->parent_synth_type = parent_synth_type;

    base->num_patterns = 1;
    base->morph_mode = false;
    base->morph_every_n_loops = 0;
    base->morph_generation = 0;
    base->max_generation = 0;

    base->multi_pattern_mode = true;
    base->cur_pattern_iteration = 1;
    for (int i = 0; i < MAX_NUM_MIDI_LOOPS; i++)
        base->pattern_multiloop_count[i] = 1;

    base->sample_rate = 44100;
    base->sample_rate_counter = 0;

    base->m_generate_src = -99;
    base->last_midi_note = 60;
    base->root_midi_note = 60;
    base->sustain_note_ms = 200;
    base->live_code_mode = true;
}

void synthbase_generate_pattern(synthbase *base)
{
    if (mixer_is_valid_seq_gen_num(mixr, base->m_generate_src))
    {
        synthbase_stop(base);
        sequence_generator *sg =
            mixr->sequence_generators[base->m_generate_src];
        uint16_t left_bits = sg->generate(sg, NULL);
        uint16_t right_bits = sg->generate(sg, NULL);

        int32_t ored_bits = (left_bits << 16) | right_bits;
        // print_bin_num(ored_bits);

        int patternlen = 32;
        for (int i = 0; i < patternlen; i++)
        {
            int shift_by = patternlen - 1 - i;
            if (ored_bits & 1 << shift_by)
            {
                synthbase_add_note(base, base->cur_pattern, i,
                                   base->root_midi_note);
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

void synthbase_pattern_to_string(synthbase *base, int pattern_num,
                                 wchar_t patternstr[33])
{
    int cur_quart = 0;
    for (int i = 0; i < PPBAR; i += PPSIXTEENTH)
    {
        patternstr[cur_quart] = sparkchars[0];
        for (int j = i; j < (i + PPSIXTEENTH); j++)
        {
            if (base->patterns[pattern_num][j].event_type == MIDI_ON)
            {
                patternstr[cur_quart] = sparkchars[5];
            }
        }
        cur_quart++;
    }
}

// sound generator interface //////////////
void synthbase_status(synthbase *base, wchar_t *status_string)
{
    swprintf(status_string, MAX_PS_STRING_SZ,
             L"\n      Multi: %s Curpattern:%d"
             "generate:%d gen_src:%d gen_every_n:%d \n"
             "      morph:%d morph_gen:%d morph_every_n:%d "
             "root_midi_note:%d last_midi_note:%d sustain_note_ms:%d",

             base->multi_pattern_mode ? "true" : "false", base->cur_pattern,
             base->generate_mode, base->m_generate_src,
             base->generate_every_n_loops, base->morph_mode,
             base->morph_generation, base->morph_every_n_loops,
             base->root_midi_note, base->last_midi_note, base->sustain_note_ms);

    for (int i = 0; i < base->num_patterns; i++)
    {
        wchar_t patternstr[33] = {0};
        wchar_t scratch[128] = {0};
        synthbase_pattern_to_string(base, i, patternstr);
        swprintf(scratch, 127, L"\n      [%d]  %ls  numloops: %d", i,
                 patternstr, base->pattern_multiloop_count[i]);
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
        if (base->multi_pattern_mode && base->num_patterns > 1)
        {
            // synthbase_stop(base);
            base->cur_pattern_iteration--;
            if (base->cur_pattern_iteration == 0)
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
            midi_event ev = base->patterns[base->cur_pattern][idx];
            midi_parse_midi_event(parent, ev);
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
    if (pattern_num >= 0 && pattern_num < ms->num_patterns)
        return true;
    return false;
}

void synthbase_set_morph_mode(synthbase *ms, bool b)
{
    ms->morph_mode = b;
    synthbase_set_backup_mode(ms, b);
}

void synthbase_set_generate_src(synthbase *b, int src)
{
    if (mixer_is_valid_seq_gen_num(mixr, src))
        b->m_generate_src = src;
}

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
    printf("BASS! how low can you go!\n");
    synthbase *ms = (synthbase *)self;
    ms->num_patterns = num_patterns;
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

void synthbase_add_note(synthbase *ms, int pattern_num, int step, int midi_note)
{
    int mstep = step * PPSIXTEENTH;
    synthbase_add_micro_note(ms, pattern_num, mstep, midi_note);
}

void synthbase_add_micro_note(synthbase *ms, int pattern_num, int mstep,
                              int midi_note)
{
    if (is_valid_pattern_num(ms, pattern_num) && mstep < PPBAR)
    {
        midi_event on = new_midi_event(MIDI_ON, midi_note, 128);

        if (ms->live_code_mode)
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
    // minisynth_morph(ms);
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
        base->last_midi_note = base->root_midi_note;
        break;
    case (1):
        base->last_midi_note = base->root_midi_note + 4;
        break;
    case (2):
        base->last_midi_note = base->root_midi_note + 7;
        break;
    }
}

void synthbase_set_root_key(synthbase *base, int root_key)
{
    base->root_midi_note = root_key;
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
            base->patterns[pattern_num][i] = pattern[i];
        }
    }
}
