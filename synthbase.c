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

void synthbase_init(synthbase *base)
{
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
            base->melodies[i][j] = NULL;
        }
    }
}

void synthbase_free_melodies(synthbase *base)
{
    for (int i = 0; i < MAX_NUM_MIDI_LOOPS; i++)
    {
        for (int j = 0; j < PPNS; j++)
        {
            if (base->melodies[i][j] != NULL)
            {
                midi_event_free(base->melodies[i][j]);
                base->melodies[i][j] = NULL;
            }
        }
    }
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

    // printf("MAX NOTES %d MAX STEPS: %d\n", max_notes, max_steps);
    int rand_num_notes = (rand() % max_notes);
    if (rand_num_notes == 0)
        rand_num_notes = 1;
    int generated_melody_note_num[NUM_COMPAT_NOTES];
    for (int i = 0; i < NUM_COMPAT_NOTES; i++)
        generated_melody_note_num[i] = -99;
    int generated_melody_note_num_idx = 0;
    while (generated_melody_note_num_idx < rand_num_notes)
    {
        int randy = rand() % (NUM_COMPAT_NOTES);
        if (!is_int_member_in_array(randy, generated_melody_note_num,
                                    NUM_COMPAT_NOTES))
        {
            generated_melody_note_num[generated_melody_note_num_idx++] = randy;
        }
    }

    for (int i = 0; i < NUM_COMPAT_NOTES; i++)
    {
        if (generated_melody_note_num[i] != -99)
        {
            int idx = generated_melody_note_num[i];

            int rand_steps = (rand() % max_steps) + 1;
            int bitpattern = create_euclidean_rhythm(rand_steps, 32);
            if (rand() % 2 == 1)
                bitpattern = shift_bits_to_leftmost_position(bitpattern, 32);

            for (int i = 31; i >= 0; i--)
            {
                if (bitpattern & 1 << i)
                {
                    synthbase_add_note(
                        base, melody_num, 31 - i,
                        key_midi_mapping[compat_keys[mixr->key][idx]]); // THIS!
                }
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

void synthbase_dupe_melody(midi_event **from, midi_event **to)
{
    for (int i = 0; i < PPNS; i++)
    {
        if (to[i] != NULL)
        {
            midi_event_free(to[i]);
            to[i] = NULL;
        }
        if (from[i] != NULL)
        {
            midi_event *ev = from[i];
            to[i] =
                new_midi_event(ev->tick, ev->event_type, ev->data1, ev->data2);
        }
    }
}

void synthbase_switch_melody(synthbase *ms, unsigned int melody_num)
{
    if (melody_num < (unsigned)ms->num_melodies)
    {
        ms->cur_melody = melody_num;
    }
}

void synthbase_reset_melody_all(synthbase *ms)
{
    for (int i = 0; i < ms->num_melodies; i++)
    {
        synthbase_reset_melody(ms, i);
    }
}

void synthbase_reset_melody(synthbase *ms, unsigned int melody_num)
{
    if (melody_num < (unsigned)ms->num_melodies)
    {
        for (int i = 0; i < PPNS; i++)
        {
            if (ms->melodies[melody_num][i] != NULL)
            {
                midi_event *tmp = ms->melodies[melody_num][i];
                ms->melodies[melody_num][i] = NULL;
                free(tmp);
            }
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
            if (base->melodies[melody_num][j] != NULL &&
                base->melodies[melody_num][j]->event_type == 144)
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
             L"\n      Multi: %s, CurMelody:%d",
             base->multi_melody_mode ? "true" : "false", base->cur_melody);

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

int synthbase_gennext(synthbase *base)
{

    if (mixr->sixteenth_note_tick != base->tick)
    {
        base->tick = mixr->sixteenth_note_tick;
        if (base->tick % 32 == 0)
        {
            if (base->generate_mode)
            {
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
                    {
                        synthbase_set_backup_mode(base, false);
                    }
                }
                else if (base->max_generation > 0)
                {
                    if (base->morph_generation >= base->max_generation)
                    {
                        base->morph_generation = 0;
                        synthbase_set_generate_mode(base, false);
                        synthbase_set_backup_mode(base, false);
                    }
                }
                else
                {
                    synthbase_generate_melody(base, 0, 0, 0);
                }
                base->generate_generation++;
            }
        }
    }

    if (mixr->is_midi_tick)
    {
        int idx = mixr->midi_tick % PPNS;
        // top of the base loop, which is two bars, check if we need to
        // progress
        // to next loop
        if (idx == 0)
        {
            if (base->multi_melody_mode && base->num_melodies > 1)
            {
                base->cur_melody_iteration--;
                if (base->cur_melody_iteration == 0)
                {
                    minisynth_midi_note_off((minisynth *)base, 0, 0, true);

                    int next_melody =
                        (base->cur_melody + 1) % base->num_melodies;

                    base->cur_melody = next_melody;
                    base->cur_melody_iteration =
                        base->melody_multiloop_count[base->cur_melody];
                }
            }
        }

        if (base->melodies[base->cur_melody][idx] != NULL)
        {
            return idx;
        }
    }
    return -1;
}

midi_event **synthbase_get_midi_loop(synthbase *self)
{
    return self->melodies[self->cur_melody];
}

int synthbase_add_event(synthbase *base, int melody_num, midi_event *ev)
{
    int tick = ev->tick;
    while (base->melodies[melody_num][tick] != NULL)
    {
        if (mixr->debug_mode)
            printf("Gotsz a tick already - bump!\n");
        tick++;
        if (tick == PPNS) // wrap around
            tick = 0;
    }
    ev->tick = tick;
    base->melodies[melody_num][tick] = ev;
    return tick;
}

void synthbase_clear_melody_ready_for_new_one(synthbase *ms, int melody_num)
{
    for (int i = 0; i < PPNS; i++)
    {
        if (ms->melodies[melody_num][i] != NULL)
        {
            midi_event *ev = ms->melodies[melody_num][i];
            if (ev->event_type == 144)
            {
                free(ms->melodies[melody_num][i]);
                ms->melodies[melody_num][i] = NULL;
            }
        }
    }
}

midi_event **synthbase_copy_midi_loop(synthbase *self, int melody_num)
{
    if (melody_num >= self->num_melodies)
    {
        printf("Dingjie!\n");
        return NULL;
    }
    // midi_event_loop defined in midimaaan.h
    midi_event **new_midi_events_loop =
        (midi_event **)calloc(PPNS, sizeof(midi_event *));
    for (int i = 0; i < PPNS; i++)
    {
        if (self->melodies[melody_num][i] != NULL)
        {
            midi_event *ev = self->melodies[melody_num][i];
            new_midi_events_loop[i] =
                new_midi_event(ev->tick, ev->event_type, ev->data1, ev->data2);
        }
    }

    return new_midi_events_loop;
}

void synthbase_add_midi_loop(synthbase *ms, midi_event **events, int melody_num)
{
    if (melody_num >= MAX_NUM_MIDI_LOOPS)
    {
        printf("Dingjie!\n");
        return;
    }
    for (int i = 0; i < PPNS; i++)
    {
        if (events[i] != NULL)
            ms->melodies[melody_num][i] = events[i];
    }
    ms->num_melodies++;
    ms->cur_melody++;
    free(events); // get rid of container
    printf("Added new Melody\n");
}

void synthbase_replace_midi_loop(synthbase *ms, midi_event **events,
                                 int melody_num)
{
    if (melody_num >= MAX_NUM_MIDI_LOOPS)
    {
        printf("Dingjie!\n");
        return;
    }
    for (int i = 0; i < PPNS; i++)
    {
        if (ms->melodies[melody_num][i] != NULL)
        {
            free(ms->melodies[melody_num][i]);
            ms->melodies[melody_num][i] = NULL;
        }
        if (events[i] != NULL)
            ms->melodies[melody_num][i] = events[i];
    }
    free(events); // get rid of container
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
    midi_event **orig_loop = synthbase_copy_midi_loop(ms, melody_num);

    midi_event **new_midi_events_loop =
        (midi_event **)calloc(PPNS, sizeof(midi_event *));

    for (int i = 0; i < PPNS; i++)
    {
        if (orig_loop[i] != NULL)
        {
            midi_event *ev = orig_loop[i];
            int new_tick = (ev->tick + (sixteenth * sixteenth_of_loop)) % PPNS;
            printf("Old tick: %d with new: %d\n", ev->tick, new_tick);
            new_midi_events_loop[new_tick] =
                new_midi_event(new_tick, ev->event_type, ev->data1, ev->data2);
        }
    }
    free(orig_loop);
    synthbase_replace_midi_loop(ms, new_midi_events_loop, melody_num);
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

void synthbase_set_backup_mode(synthbase *ms, bool b)
{
    if (b)
    {
        synthbase_dupe_melody(ms->melodies[0],
                              ms->backup_melody_while_getting_crazy);
        // ms->m_settings_backup_while_getting_crazy = ms->m_settings;
        ms->multi_melody_mode = false;
        ms->cur_melody = 0;
    }
    else
    {
        synthbase_dupe_melody(ms->backup_melody_while_getting_crazy,
                              ms->melodies[0]);
        // ms->m_settings = ms->m_settings_backup_while_getting_crazy;
        ms->multi_melody_mode = true;
    }
}

void synthbase_morph(synthbase *ms)
{
    int midinotes_seen[10] = {0};
    int midinotes_seen_idx = 0;
    for (int i = 0; i < PPNS; i++)
    {

        midi_event *e = ms->melodies[0][i];

        if (e != NULL &&
            !is_int_member_in_array(e->data1, midinotes_seen, 10) &&
            midinotes_seen_idx < 10)
        {
            midinotes_seen[midinotes_seen_idx++] = e->data1;
        }

        int rand_num = (rand() % 3) + 1;
        int rand_num2 = (rand() % 3) + 1;
        int rand_num3 = rand() % 2;
        int rand_num4 = rand() % 2;

        int num_notes = synthbase_get_num_notes(ms);

        if (e != NULL && e->event_type == 144)
        {
            // printf("KEY! %d - %d %d %d\n", i, e->event_type,
            // e->data1,
            //       e->data2);

            int randy = rand() % 4;
            int j = 0;
            int new_note_num = 0;
            switch (randy)
            {
            case 0:
                if (num_notes > 3)
                {
                    // printf("Removing note\n");
                    synthbase_rm_micro_note(ms, 0, i);
                }
                break;
            case 1:
                if (num_notes < 15)
                {
                    // printf("Duping note\n");
                    for (j = 1; j < rand_num; j++)
                    {
                        int next_note =
                            (i + rand_num2 * PPSIXTEENTH * j) % PPNS;
                        synthbase_add_micro_note(ms, 0, next_note, e->data1);
                    }
                }
                break;
            case 2:
                if (rand_num3)
                    if (rand_num4)
                        new_note_num = e->data1 + 7;
                    else
                        new_note_num = e->data1 - 7;
                else if (rand_num4)
                    new_note_num = e->data1 + 5;
                else
                    new_note_num = e->data1 - 5;

                if (new_note_num > 0)
                {
                    synthbase_rm_micro_note(ms, 0, i);
                    synthbase_add_micro_note(ms, 0, i, new_note_num);
                }
                break;
            case 3: // no-op - leave the note as is
            default:
                break;
            }
        }
    }
    // synthbase_print_melodies(ms);
    // synthbase_update(ms);
}

int synthbase_get_num_notes(synthbase *ms)
{
    int notecount = 0;
    for (int i = 0; i < ms->num_melodies; i++)
    {
        for (int j = 0; j < PPNS; j++)
        {
            midi_event *e = ms->melodies[i][j];
            if (e != NULL && e->event_type == 144)
                notecount++;
        }
    }
    return notecount;
}

int synthbase_get_notes_from_melody(midi_event **melody,
                                    int return_midi_notes[10])
{
    int idx = 0;
    for (int i = 0; i < PPNS; i++)
    {
        if (melody[i] != NULL)
        {
            midi_event *e = melody[i];
            if (e->event_type == 144)
            { // note on
                if (!is_int_member_in_array(e->data1, return_midi_notes, 10))
                {
                    return_midi_notes[idx++] = e->data1;
                    if (idx == 10)
                        return idx;
                }
            }
        }
    }
    return idx;
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
    {
        printf("Pattern Num %d\n", i);
        midi_event **melody = ms->melodies[i];
        midi_melody_print(melody);
    }
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
        midi_event *on = new_midi_event(mstep, 144, midi_note, 128);
        // int note_off_tick = (mstep + (PPSIXTEENTH * 4 - 7)) % PPNS;
        int note_off_tick = (mstep + (PPSIXTEENTH * 4 - 7)) %
                            //(int)(PPSIXTEENTH *
                            // ms->m_settings.m_sustain_time_sixteenth) - 7) %
                            PPNS;
        midi_event *off = new_midi_event(note_off_tick, 128, midi_note, 128);

        int final_note_off_tick = synthbase_add_event(ms, pattern_num, off);
        on->tick_off = final_note_off_tick;
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
        if (ms->melodies[pat_num][tick] != NULL)
        {
            midi_event *ev = ms->melodies[ms->cur_melody][tick];
            // synthbase_midi_note_off(ms, ev->data1, ev->data2, false);
            ms->melodies[ms->cur_melody][tick] = NULL;
            int tick_off = ev->tick_off;
            free(ev);
            // printf("Deleted midi event at tick %d\n", tick);
            if (tick_off)
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
        if (ms->melodies[pattern_num][fromstep] != NULL)
        {
            synthbase_rm_micro_note(ms, pattern_num, tostep);
            ms->melodies[pattern_num][tostep] =
                ms->melodies[pattern_num][fromstep];
            ms->melodies[pattern_num][fromstep] = NULL;
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
            midi_event *ev = new_midi_event(tick, status, midi_note, midi_vel);
            synthbase_add_event(base, base->cur_melody, ev);
        }
    }

    synthbase_set_multi_melody_mode(base, true);
    fclose(fp);
}
