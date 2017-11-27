#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "defjams.h"
#include "mixer.h"
#include "sequencer_utils.h"
#include "synthbase.h"
#include "utils.h"

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

    base->num_melodies = 1;
    base->morph_mode = false;
    base->morph_every_n_loops = 0;
    base->morph_generation = 0;
    base->max_generation = 0;

    base->multi_melody_mode = true;
    base->cur_melody_iteration = 1;

    for (int i = 0; i < MAX_NUM_MIDI_LOOPS; i++)
    {
        base->melody_multiloop_count[i] = 1;
        for (int j = 0; j < PPNS; j++)
        {
            base->melodies[i][j].tick = -1;
        }
    }
    for (int i = 0; i < PPNS; ++i)
        base->backup_melody_while_getting_crazy[i].tick = -1;
}

void synthbase_generate_melody(synthbase *base, int melody_num, int max_notes,
                               int max_steps)
{
    if (!is_valid_melody_num(base, melody_num))
    {
        printf("Not a valid melody number\n");
        return;
    }

    synthbase_reset_melody(base, melody_num);

    if (max_notes == 0)
        max_notes = 5;
    if (max_steps == 0)
        max_steps = 9;

    // 1. generate NOTES/MELODY
    int rand_num_notes = (rand() % max_notes);
    if (rand_num_notes == 0)
        rand_num_notes = 2;

    int generated_melody_note_num[rand_num_notes];
    for (int i = 0; i < rand_num_notes; i++)
        generated_melody_note_num[i] = -99;
    int generated_melody_note_num_idx = 0;

    while (generated_melody_note_num_idx < rand_num_notes)
    {
        int randy = rand() % (NUM_COMPAT_NOTES);
        if (!is_int_member_in_array(randy, generated_melody_note_num,
                                    rand_num_notes))
        {
            generated_melody_note_num[generated_melody_note_num_idx++] = randy;
        }
    }
    // printf("Num notes:%d ", rand_num_notes);
    // for (int i = 0; i < rand_num_notes; ++i)
    //    printf("%d(%s) ", generated_melody_note_num[i],
    //           key_names[generated_melody_note_num[i]]);
    // printf("\n");
    // END NOTES/MELODY

    // 2. PLACEMENT OF NOTES
    int rand_steps = (rand() % max_steps / 2);
    int num_steps = rand_steps;
    int bitpattern = create_euclidean_rhythm(rand_steps, 32);
    if (rand() % 2 == 1)
        bitpattern = shift_bits_to_leftmost_position(bitpattern, 32);
    rand_steps = (rand() % max_steps / 2);
    num_steps += rand_steps;
    bitpattern += create_euclidean_rhythm(num_steps, 32);

    // print_bin_num(bitpattern);
    int num_hits = how_many_bits_in_num(bitpattern);
    // printf("I gots %d hits\n", num_hits);

    int largest_divisor = num_hits / rand_num_notes;
    int rem = num_hits % rand_num_notes;
    int generated_melody_note_num_hits[rand_num_notes];
    for (int i = 0; i < rand_num_notes; ++i)
        generated_melody_note_num_hits[i] = largest_divisor;

    if (rem > 0)
        generated_melody_note_num_hits[rand() % rand_num_notes] += rem;

    // for (int i = 0 ; i < rand_num_notes; i++)
    //    printf("Playing %s %d times\n",
    //    key_names[generated_melody_note_num[i]],
    //    generated_melody_note_num_hits[i]);

    int cur_key = 0;
    int cur_key_count = 0;
    for (int i = 31; i >= 0; i--)
    {
        if (bitpattern & 1 << i)
        {
            synthbase_add_note(
                base, melody_num, 31 - i,
                key_midi_mapping[compat_keys[mixr->key][cur_key]]);
            cur_key_count++;
            if (cur_key_count >= generated_melody_note_num_hits[cur_key])
            {
                cur_key++;
                cur_key_count = 0;
            }
        }
    }
}

void synthbase_set_multi_melody_mode(synthbase *ms, bool melody_mode)
{
    ms->multi_melody_mode = melody_mode;
    ms->cur_melody_iteration = ms->melody_multiloop_count[ms->cur_melody];
}

void synthbase_set_melody_loop_num(synthbase *self, int melody_num,
                                   int loop_num)
{
    self->melody_multiloop_count[melody_num] = loop_num;
}

int synthbase_add_melody(synthbase *ms) { return ms->num_melodies++; }

void synthbase_dupe_melody(midi_events_loop *from, midi_events_loop *to)
{
    for (int i = 0; i < PPNS; i++)
        (*to)[i] = (*from)[i];
}

void synthbase_switch_melody(synthbase *ms, unsigned int melody_num)
{
    if (melody_num < (unsigned)ms->num_melodies)
        ms->cur_melody = melody_num;
}

void synthbase_reset_melody_all(synthbase *ms)
{
    for (int i = 0; i < MAX_NUM_MIDI_LOOPS; i++)
    {
        synthbase_reset_melody(ms, i);
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

void synthbase_reset_melody(synthbase *base, unsigned int melody_num)
{
    synthbase_stop(base);

    if (melody_num < MAX_NUM_MIDI_LOOPS)
    {
        for (int i = 0; i < PPNS; i++)
        {
            base->melodies[melody_num][i].tick = -1;
        }
    }
}

void synthbase_melody_to_string(synthbase *base, int melody_num,
                                wchar_t melodystr[33])
{
    int cur_quart = 0;
    for (int i = 0; i < PPNS; i += PPSIXTEENTH)
    {
        melodystr[cur_quart] = sparkchars[0];
        for (int j = i; j < (i + PPSIXTEENTH); j++)
        {
            if (base->melodies[melody_num][j].tick != -1 &&
                base->melodies[melody_num][j].event_type == 144)
            { // 144 is midi note on
                melodystr[cur_quart] = sparkchars[5];
            }
        }
        cur_quart++;
    }
}

// sound generator interface //////////////
void synthbase_status(synthbase *base, wchar_t *status_string)
{
    swprintf(status_string, MAX_PS_STRING_SZ,
             L"\n      Multi: %s CurMelody:%d"
             "generate:%d gen_gen:%d gen_every_n:%d \n"
             "      morph:%d morph_gen:%d morph_every_n:%d",

             base->multi_melody_mode ? "true" : "false", base->cur_melody,
             base->generate_mode, base->generate_generation,
             base->generate_every_n_loops, base->morph_mode,
             base->morph_generation, base->morph_every_n_loops);

    for (int i = 0; i < base->num_melodies; i++)
    {
        wchar_t melodystr[33] = {0};
        wchar_t scratch[128] = {0};
        synthbase_melody_to_string(base, i, melodystr);
        swprintf(scratch, 127, L"\n      [%d]  %ls  numloops: %d", i, melodystr,
                 base->melody_multiloop_count[i]);
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
    case (TIME_MIDI_TICK):
        idx = mixr->timing_info.midi_tick % PPNS;
        if (base->melodies[base->cur_melody][idx].tick != -1)
        {
            midi_event ev = base->melodies[base->cur_melody][idx];
            midi_parse_midi_event(parent, ev);
        }

        // top of the base loop, which is two bars, check if we need to
        // progress to next loop
        if (idx == 0)
        {
            if (base->multi_melody_mode && base->num_melodies > 1)
            {
                synthbase_stop(base);
                base->cur_melody_iteration--;
                if (base->cur_melody_iteration == 0)
                {
                    if (base->parent_synth_type == MINISYNTH_TYPE)
                        minisynth_midi_note_off((minisynth *)parent, 0, 0,
                                                true /* all notes off */);
                    else if (base->parent_synth_type == DXSYNTH_TYPE)
                        dxsynth_midi_note_off((dxsynth *)parent, 0, 0,
                                              true /* all notes off */);

                    int next_melody =
                        (base->cur_melody + 1) % base->num_melodies;

                    base->cur_melody = next_melody;
                    base->cur_melody_iteration =
                        base->melody_multiloop_count[base->cur_melody];
                }
            }

            if (base->generate_mode)
            {
                synthbase_stop(base);
                if (base->generate_every_n_loops > 0)
                {
                    if (base->generate_generation %
                            base->generate_every_n_loops ==
                        0)
                    {
                        synthbase_set_backup_mode(base, true);
                        synthbase_generate_melody(base, 0, 0, 0);
                    }
                    else
                        synthbase_set_backup_mode(base, false);
                }
                else if (base->max_generation > 0)
                {
                    if (base->generate_generation >= base->max_generation)
                    {
                        base->generate_generation = 0;
                        synthbase_set_generate_mode(base, false);
                        synthbase_set_backup_mode(base, false);
                    }
                }
                else
                    synthbase_generate_melody(base, 0, 0, 0);

                base->generate_generation++;
            }

            if (base->morph_mode)
            {
                synthbase_stop(base);
                if (base->morph_every_n_loops > 0)
                {
                    if (base->morph_generation % base->morph_every_n_loops == 0)
                    {
                        synthbase_set_backup_mode(base, true);
                        synthbase_morph(base);
                    }
                    else
                        synthbase_set_backup_mode(base, false);
                }
                else if (base->max_generation > 0)
                {
                    if (base->morph_generation >= base->max_generation)
                    {
                        base->morph_generation = 0;
                        synthbase_set_morph_mode(base, false);
                        synthbase_set_backup_mode(base, false);
                    }
                }
                else
                    synthbase_morph(base);

                base->morph_generation++;
            }
        }
        break;
    }
}

int synthbase_add_event(synthbase *base, int melody_num, midi_event ev)
{
    int tick = ev.tick;
    while (base->melodies[melody_num][tick].tick != -1)
    {
        if (mixr->debug_mode)
            printf("Gotsz a tick already - bump!\n");
        tick++;
        if (tick == PPNS) // wrap around
            tick = 0;
    }
    ev.tick = tick;
    base->melodies[melody_num][tick] = ev;
    return tick;
}

void synthbase_clear_melody_ready_for_new_one(synthbase *ms, int melody_num)
{
    for (int i = 0; i < PPNS; i++)
    {
        ms->melodies[melody_num][i].tick = -1;
    }
}

void synthbase_copy_midi_loop(synthbase *self, int melody_num,
                              midi_events_loop *target_loop)
{
    if (melody_num >= self->num_melodies)
    {
        printf("Dingjie!\n");
        return;
    }
    // midi_event_loop defined in midimaaan.h
    for (int i = 0; i < PPNS; i++)
    {
        (*target_loop)[i].tick = -1; // clear old contents
        if (self->melodies[melody_num][i].tick != -1)
        {
            (*target_loop)[i] = self->melodies[melody_num][i];
        }
    }
}

void synthbase_replace_midi_loop(synthbase *ms, midi_events_loop *source_loop,
                                 int melody_num)
{
    if (melody_num >= MAX_NUM_MIDI_LOOPS)
    {
        printf("Dingjie!\n");
        return;
    }
    for (int i = 0; i < PPNS; i++)
    {
        ms->melodies[melody_num][i].tick = -1;
        if ((*source_loop)[i].tick != -1)
            ms->melodies[melody_num][i] = (*source_loop)[i];
    }
    printf("Replaced Melody %d\n", melody_num);
}

void synthbase_nudge_melody(synthbase *ms, int melody_num, int sixteenth)
{
    if (sixteenth >= 16)
    {
        printf("Nah, mate, nudge needs to be less than 16\n");
        return;
    }
    int sixteenth_of_loop = PPNS / 16.0;
    midi_events_loop loop_copy;

    synthbase_copy_midi_loop(ms, melody_num, &loop_copy);

    midi_events_loop new_loop;
    for (int i = 0; i < PPNS; i++)
    {
        if (loop_copy[i].tick != -1)
        {
            int new_tick =
                (loop_copy[i].tick + (sixteenth * sixteenth_of_loop)) % PPNS;
            new_loop[new_tick] = loop_copy[i];
            new_loop[new_tick].tick = new_tick;
        }
    }

    synthbase_replace_midi_loop(ms, &new_loop, melody_num);
}

bool is_valid_melody_num(synthbase *ms, int melody_num)
{
    if (melody_num < ms->num_melodies)
    {
        return true;
    }
    return false;
}

void synthbase_set_morph_mode(synthbase *ms, bool b)
{
    ms->morph_mode = b;
    synthbase_set_backup_mode(ms, b);
}

void synthbase_set_generate_mode(synthbase *ms, bool b)
{
    ms->generate_mode = b;
    synthbase_set_backup_mode(ms, b);
}

void synthbase_set_backup_mode(synthbase *base, bool b)
{
    if (b)
    {
        synthbase_dupe_melody(&base->melodies[0],
                              &base->backup_melody_while_getting_crazy);
        // base->m_settings_backup_while_getting_crazy = base->m_settings;
        base->multi_melody_mode = false;
        base->cur_melody = 0;
    }
    else
    {
        synthbase_dupe_melody(&base->backup_melody_while_getting_crazy,
                              &base->melodies[0]);
        // base->m_settings = base->m_settings_backup_while_getting_crazy;
        base->multi_melody_mode = true;
    }
}

void synthbase_morph(synthbase *base)
{
    synthbase_reset_melody(base, 0);

    int max_steps = 10;

    int midi_notes[10];
    int num_notes = synthbase_get_notes_from_melody(
        &base->backup_melody_while_getting_crazy, midi_notes);

    if (num_notes == 0)
    {
        printf("Whoa nellie! nae notes\n");
        return;
    }

    int rand_steps = (rand() % max_steps / 2);
    int num_steps = rand_steps;
    int bitpattern = create_euclidean_rhythm(rand_steps, 32);
    if (rand() % 2 == 1)
        bitpattern = shift_bits_to_leftmost_position(bitpattern, 32);
    rand_steps = (rand() % max_steps / 2);
    num_steps += rand_steps;
    bitpattern += create_euclidean_rhythm(num_steps, 32);

    // print_bin_num(bitpattern);
    int num_hits = how_many_bits_in_num(bitpattern);
    // printf("I gots %d hits\n", num_hits);

    int largest_divisor = num_hits / num_notes;
    int rem = num_hits % num_notes;
    int generated_melody_note_num_hits[num_notes];
    for (int i = 0; i < num_notes; ++i)
        generated_melody_note_num_hits[i] = largest_divisor;

    if (rem > 0)
        generated_melody_note_num_hits[rand() % num_notes] += rem;

    // for (int i = 0 ; i < rand_num_notes; i++)
    //    printf("Playing %s %d times\n",
    //    key_names[generated_melody_note_num[i]],
    //    generated_melody_note_num_hits[i]);

    int cur_key = 0;
    int cur_key_count = 0;
    for (int i = 31; i >= 0; i--)
    {
        if (bitpattern & 1 << i)
        {
            synthbase_add_note(
                base, 0, 31 - i,
                key_midi_mapping[compat_keys[mixr->key][cur_key]]);
            cur_key_count++;
            if (cur_key_count >= generated_melody_note_num_hits[cur_key])
            {
                cur_key++;
                cur_key_count = 0;
            }
        }
    }
}

int synthbase_get_num_notes(synthbase *ms)
{
    int notecount = 0;
    for (int i = 0; i < ms->num_melodies; i++)
        for (int j = 0; j < PPNS; j++)
            if (ms->melodies[i][j].tick != -1 &&
                ms->melodies[i][j].event_type == 144)
                notecount++;
    return notecount;
}

int synthbase_get_notes_from_melody(midi_events_loop *loop,
                                    int return_midi_notes[10])
{
    int idx = 0;
    for (int i = 0; i < PPNS; i++)
    {
        midi_event ev = (*loop)[i];
        if (ev.tick != -1)
        {
            if (ev.event_type == 144)
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
    }
    return idx; // num notes
}

int synthbase_get_num_tracks(void *self)
{
    synthbase *ms = (synthbase *)self;
    return ms->num_melodies;
}

void synthbase_make_active_track(void *self, int pattern_num)
{
    synthbase *ms = (synthbase *)self;
    ms->cur_melody =
        pattern_num; // TODO - standardize - PATTERN? TRACK? MELODY?!?!
}

void synthbase_print_melodies(synthbase *ms)
{
    for (int i = 0; i < ms->num_melodies; i++)
        midi_melody_print(&ms->melodies[i]);
}

void synthbase_add_note(synthbase *ms, int pattern_num, int step, int midi_note)
{
    int mstep = step * PPSIXTEENTH;
    synthbase_add_micro_note(ms, pattern_num, mstep, midi_note);
}

void synthbase_add_micro_note(synthbase *ms, int pattern_num, int mstep,
                              int midi_note)
{
    if (is_valid_melody_num(ms, pattern_num) && mstep < PPNS)
    {
        // printf("New Notes!! %d - %d\n", mstep, midi_note);
        midi_event on = new_midi_event(mstep, 144, midi_note, 128);
        // int note_off_tick = (mstep + (PPSIXTEENTH * 4 - 7)) % PPNS;
        int note_off_tick = (mstep + (PPSIXTEENTH * 4 - 7)) %
                            //(int)(PPSIXTEENTH *
                            // ms->m_settings.m_sustain_time_sixteenth) - 7) %
                            PPNS;
        midi_event off = new_midi_event(note_off_tick, 128, midi_note, 128);

        int final_note_off_tick = synthbase_add_event(ms, pattern_num, off);
        on.tick_off = final_note_off_tick;
        synthbase_add_event(ms, pattern_num, on);
    }
    else
    {
        printf("Adding MICRO note - not valid melody-num(%d) || step no "
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
    if (is_valid_melody_num(ms, pat_num) && tick < PPNS)
    {
        if (ms->melodies[pat_num][tick].tick != -1)
        {
            ms->melodies[ms->cur_melody][tick].tick = -1;
            int tick_off = ms->melodies[ms->cur_melody][tick].tick_off;
            if (tick_off >= 0)
                synthbase_rm_micro_note(ms, pat_num, tick_off);
        }
        else
        {
            if (mixr->debug_mode)
                printf("Not a valid midi event at tick: %d\n", tick);
        }
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
    if (is_valid_melody_num(ms, pattern_num))
    {
        if (ms->melodies[pattern_num][fromstep].tick != -1)
        {
            ms->melodies[pattern_num][tostep] =
                ms->melodies[pattern_num][fromstep];
            ms->melodies[pattern_num][fromstep].tick = -1;
        }
        else
        {
            printf("Woof, cannae move micro note - either fromstep(%d) "
                   "is NULL or "
                   "tostep(%d) is not less than %d\n",
                   fromstep, tostep, PPNS);
        }
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
        int max_tick = PPNS * base->num_melodies;
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
                    synthbase_add_melody(base);
                    tick = tick % PPNS;
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
            midi_event ev = new_midi_event(tick, status, midi_note, midi_vel);
            synthbase_add_event(base, base->cur_melody, ev);
        }
    }

    synthbase_set_multi_melody_mode(base, true);
    fclose(fp);
}

int synthbase_change_octave_melody(synthbase *base, int melody_num,
                                   int direction)
{
    printf("Changing octave of %d - direction: %s\n", melody_num,
           direction == 1 ? "UP" : "DOWN");
    if (is_valid_melody_num(base, melody_num))
    {
        for (int i = 0; i < PPNS; i++)
            if (base->melodies[melody_num][i].tick != -1)
            {
                int new_midi_num = base->melodies[melody_num][i].data1;
                if (direction == 1) // up
                    new_midi_num += 12;
                else
                    new_midi_num -= 12;
                if (new_midi_num >= 0 && new_midi_num < 128)
                    base->melodies[melody_num][i].data1 = new_midi_num;
            }
    }
    else
        return 1;

    return 0;
}
