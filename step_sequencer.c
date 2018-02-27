#include <libgen.h>
#include <math.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <bitshift.h>
#include <defjams.h>
#include <euclidean.h>
#include <mixer.h>
#include <pattern_parser.h>
#include <sequencer_utils.h>
#include <step_sequencer.h>
#include <utils.h>

extern mixer *mixr;
extern wchar_t *sparkchars;

const double DEFAULT_AMP = 0.7;

const char *s_markov_mode[] = {"boombap", "haus", "snare"};

void seq_init(sequencer *seq)
{

    seq->sixteenth_tick =
        0; // TODO - this is less relevant, now that i also have 24th
    seq->midi_tick = 0;

    seq->num_patterns = 1;
    seq->cur_pattern = 0;
    seq->multi_pattern_mode = true;
    seq->cur_pattern_iteration = 1;
    seq->pattern_len = 16;

    for (int i = 0; i < MAX_SEQUENCER_PATTERNS; i++)
    {
        seq->pattern_num_loops[i] = 1;
        for (int j = 0; j < PPBAR; j++)
        {
            seq->pattern_position_amp[i][j] = DEFAULT_AMP;
        }
    }

    seq->randamp_on = false;
    seq->randamp_generation = 0;
    seq->randamp_every_n_loops = 0;

    seq->generate_mode = false;
    seq->generate_src = -99;
    seq->generate_generation = 0;
    seq->generate_every_n_loops = 0;
    seq->generate_max_generation = 0;

    seq->sloppiness = 0;

    seq->visualize = false;
}

bool seq_tick(sequencer *seq)
{
    if (mixr->timing_info.sixteenth_note_tick != seq->sixteenth_tick)
    {
        seq->sixteenth_tick = mixr->timing_info.sixteenth_note_tick;

        if (seq->sixteenth_tick % 16 == 0)
        {

            if (seq->multi_pattern_mode)
            {
                // printf("MULTI BEEP!\n");
                seq->cur_pattern_iteration--;
                if (seq->cur_pattern_iteration == 0)
                {
                    // printf("BEEP BEEP!\n");
                    seq->cur_pattern =
                        (seq->cur_pattern + 1) % seq->num_patterns;
                    seq->cur_pattern_iteration =
                        seq->pattern_num_loops[seq->cur_pattern];
                }
            }

            if (seq->generate_mode)
            {
                bool gen_new_pattern = false;
                if (seq->generate_every_n_loops > 0)
                {
                    if (seq->generate_generation %
                            seq->generate_every_n_loops ==
                        0)
                    {
                        seq_set_backup_mode(seq, true);
                        gen_new_pattern = true;
                    }
                    else
                    {
                        seq_set_backup_mode(seq, false);
                    }
                }
                else if (seq->generate_max_generation > 0)
                {
                    if (seq->generate_generation >=
                        seq->generate_max_generation)
                    {
                        seq->generate_generation = 0;
                        seq_set_generate_mode(seq, false);
                    }
                }
                else
                {
                    gen_new_pattern = true;
                }
                if (gen_new_pattern)
                {
                    if (seq->generate_src != -99)
                    {
                        sequence_generator *sg =
                            mixr->sequence_generators[seq->generate_src];
                        int bit_pattern_len = 16; // default
                        int bit_pattern =
                            sg->generate((void *)sg, (void *)&bit_pattern_len);

                        if (seq->visualize)
                        {
                            char bit_string[17];
                            char_binary_version_of_short(bit_pattern,
                                                         bit_string);
                            printf("New pattern: %s\n", bit_string);
                        }

                        memset(&seq->patterns[seq->cur_pattern], 0,
                               PPBAR * sizeof(midi_event));
                        convert_bit_pattern_to_step_pattern(
                            bit_pattern, bit_pattern_len,
                            (int *)&seq->patterns[seq->cur_pattern], PPBAR);
                        seq->pattern_len = bit_pattern_len;
                    }
                }
                seq->generate_generation++;
            }

            if (seq->randamp_on)
            {
                if (seq->randamp_every_n_loops > 0)
                {
                    if (seq->randamp_generation % seq->randamp_every_n_loops ==
                        0)
                    {
                        seq_set_random_sample_amp(seq, seq->cur_pattern);
                    }
                }
                else
                {
                    seq_set_random_sample_amp(seq, seq->cur_pattern);
                }
                seq->randamp_generation++;
            }
        }
        return true;
    }
    return false;
}

void pattern_char_to_pattern(sequencer *s, char *char_pattern,
                             midi_event *final_pattern)
{
    int sp_count = 0;
    char *sp, *sp_last;
    int pattern[s->pattern_len];
    char const *sep = " ";

    // extract numbers from string into pattern
    for (sp = strtok_r(char_pattern, sep, &sp_last); sp;
         sp = strtok_r(NULL, sep, &sp_last))
    {
        pattern[sp_count++] = atoi(sp);
    }
    memset(final_pattern, 0, PPBAR * sizeof(midi_event));
    int mult = PPBAR / s->pattern_len;

    for (int i = 0; i < sp_count; i++)
    {
        printf("Adding hit to midi step %d\n", pattern[i] * mult);
        final_pattern[pattern[i] * mult].event_type = MIDI_ON;
    }
}

int sloppy_weight(sequencer *s, int position)
{
    if (!s->sloppiness)
        return position;

    int sloopy =
        position; // yeah, yeah, sloppy, i know - but sloopy sounds cool.

    if (rand() % 2) // positive
        sloopy += rand() % s->sloppiness;
    else
        sloopy -= rand() % s->sloppiness;

    if (sloopy < PPBAR)
        return sloopy;
    else
        return position;
}

void seq_status(sequencer *seq, wchar_t *status_string)
{
    wchar_t pattern_details[128];
    char spattern[seq->pattern_len + 1];
    wchar_t apattern[seq->pattern_len + 1];
    for (int i = 0; i < seq->num_patterns; i++)
    {
        seq_char_binary_version_of_pattern(seq, seq->patterns[i], spattern);
        wchar_version_of_amp(seq, i, apattern);
        swprintf(pattern_details, 127,
                 L"\n           [%d] - [%s] %ls  numloops: %d Swing: %d", i,
                 spattern, apattern, seq->pattern_num_loops[i],
                 seq->pattern_num_swing_setting[i]);
        wcscat(status_string, pattern_details);
    }
}

// TODO - fix this - being lazy and want it finished NOW!
void wchar_version_of_amp(sequencer *s, int pattern_num, wchar_t *apattern)
{
    for (int i = 0; i < s->pattern_len; i++)
    {
        double amp = s->pattern_position_amp[pattern_num][i];
        int idx = (int)floor(scaleybum(0, 1.1, 0, wcslen(sparkchars), amp));
        apattern[i] = sparkchars[idx];
        // wprintf(L"\n%ls\n", sparkchars[3]);
    }
    apattern[s->pattern_len] = '\0';
}

void add_char_pattern(sequencer *s, char *pattern)
{
    pattern_char_to_pattern(s, pattern, s->patterns[s->num_patterns++]);
    s->cur_pattern++;
}

void change_char_pattern(sequencer *s, int pattern_num, char *pattern)
{
    pattern_char_to_pattern(s, pattern, s->patterns[pattern_num]);
}

void seq_set_sample_amp(sequencer *s, int pattern_num, int pattern_position,
                        double v)
{
    s->pattern_position_amp[pattern_num][pattern_position] = v;
}

void seq_set_random_sample_amp(sequencer *s, int pattern_num)
{
    for (int i = 0; i < s->pattern_len; i++)
    {
        double randy = (double)rand() / (double)RAND_MAX;
        seq_set_sample_amp(s, pattern_num, i, randy);
    }
}

void seq_set_sample_amp_from_char_pattern(sequencer *s, int pattern_num,
                                          char *amp_pattern)
{
    printf("Ooh, setting amps to %s\n", amp_pattern);

    int sp_count = 0;
    char *sp, *sp_last, *spattern[32];
    char const *sep = " ";

    printf("CHARPATT %s\n", amp_pattern);
    // extract numbers from string into spattern
    for (sp = strtok_r(amp_pattern, sep, &sp_last); sp;
         sp = strtok_r(NULL, sep, &sp_last))
    {
        spattern[sp_count++] = sp;
    }

    for (int i = 0; i < sp_count; i++)
    {
        printf("[%d] -- %s\n", i, spattern[i]);
        seq_set_sample_amp(s, pattern_num, atof(spattern[i]), 1);
    }
}

void seq_set_multi_pattern_mode(sequencer *s, bool multi)
{
    s->multi_pattern_mode = multi;
    s->cur_pattern_iteration = s->pattern_num_loops[s->cur_pattern];
}

void seq_change_num_loops(sequencer *s, int pattern_num, int num_loops)
{
    if (pattern_num < s->num_patterns && num_loops > 0)
    {
        s->pattern_num_loops[pattern_num] = num_loops;
    }
}

void seq_set_generate_mode(sequencer *s, bool b)
{
    s->generate_generation = 0;
    s->generate_mode = b;
    seq_set_backup_mode(s, b);
}

void seq_clear_pattern(sequencer *s, int pattern_num)
{
    memset(&s->patterns[pattern_num], 0, PPBAR * sizeof(midi_event));
}

void seq_set_backup_mode(sequencer *s, bool on)
{
    if (on)
    {
        memset(s->backup_pattern_while_getting_crazy, 0,
               PPBAR * sizeof(midi_event));
        memcpy(s->backup_pattern_while_getting_crazy, &s->patterns[0],
               PPBAR * sizeof(midi_event));
        s->multi_pattern_mode = false;
        s->cur_pattern = 0;
    }
    else
    {
        memset(&s->patterns[0], 0, PPBAR * sizeof(midi_event));
        memcpy(&s->patterns[0], s->backup_pattern_while_getting_crazy,
               PPBAR * sizeof(midi_event));
        s->multi_pattern_mode = true;
    }
}

void seq_set_max_generations(sequencer *s, int max)
{
    s->generate_max_generation = max;
}

void seq_set_generate_src(sequencer *s, int src) { s->generate_src = src; }

void seq_set_randamp(sequencer *s, bool b) { s->randamp_on = b; }

void seq_set_pattern_len(sequencer *s, int len)
{
    int ridiculous_size = 101;
    if (len < ridiculous_size)
        s->pattern_len = len;
}
void seq_wchar_binary_version_of_pattern(sequencer *s, midi_pattern p,
                                         wchar_t *bin_num)
{
    int incs = PPBAR / s->pattern_len;
    for (int i = 0; i < s->pattern_len; i++)
    {
        int start = i * incs;
        if (is_midi_event_in_range(start, start + incs, p))
            bin_num[i] = sparkchars[5];
        else
            bin_num[i] = sparkchars[0];
    }
    bin_num[s->pattern_len] = '\0';
}

void seq_char_binary_version_of_pattern(sequencer *s, midi_pattern p,
                                        char *bin_num)
{
    int incs = PPBAR / s->pattern_len;
    for (int i = 0; i < s->pattern_len; i++)
    {
        int start = i * incs;
        if (is_midi_event_in_range(start, start + incs, p))
            bin_num[i] = '1';
        else
            bin_num[i] = '0';
    }
    bin_num[s->pattern_len] = '\0';
}

void seq_print_pattern(sequencer *s, unsigned int pattern_num)
{
    if (seq_is_valid_pattern_num(s, pattern_num))
    {
        printf("Pattern %d\n", pattern_num);
        for (int i = 0; i < PPBAR; i++)
        {
            if (s->patterns[pattern_num][i].event_type == MIDI_ON)
            {
                printf("[%d] on\n", i);
            }
        }
    }
}

bool seq_is_valid_pattern_num(sequencer *d, int pattern_num)
{
    if (pattern_num >= 0 && pattern_num < MAX_NUM_MIDI_LOOPS)
    {
        if (pattern_num >= d->num_patterns)
            d->num_patterns = pattern_num + 1;
        return true;
    }
    return false;
}

void seq_mv_hit(sequencer *s, int pattern_num, int stepfrom, int stepto)
{
    int multi = PPBAR / s->pattern_len;

    int mstepfrom = stepfrom * multi;
    int mstepto = stepto * multi;

    seq_mv_micro_hit(s, pattern_num, mstepfrom, mstepto);
}

void seq_add_hit(sequencer *s, int pattern_num, int step)
{
    int multi = PPBAR / s->pattern_len;
    int mstep = step * multi;
    seq_add_micro_hit(s, pattern_num, mstep);
}

void seq_rm_hit(sequencer *s, int pattern_num, int step)
{
    int multi = PPBAR / s->pattern_len;
    int mstep = step * multi;
    seq_rm_micro_hit(s, pattern_num, mstep);
}

void seq_mv_micro_hit(sequencer *s, int pattern_num, int stepfrom, int stepto)
{
    // printf("SEQ mv micro\n");
    if (seq_is_valid_pattern_num(s, pattern_num))
    {
        if (s->patterns[pattern_num][stepfrom].event_type == MIDI_ON &&
            stepto < PPBAR)
        {
            s->patterns[pattern_num][stepto] =
                s->patterns[pattern_num][stepfrom];
            midi_event_clear(&s->patterns[pattern_num][stepfrom]);
        }
        else
        {
            printf("Sumthing wrong - either stepfrom(%d) is not a hit or "
                   "stepto(%d) not less than PPBAR(%d)\n",
                   stepfrom, stepto, PPBAR);
        }
    }
}

void seq_add_micro_hit(sequencer *s, int pattern_num, int step)
{
    printf("MICRO Add'ing %d\n", step);
    if (seq_is_valid_pattern_num(s, pattern_num) && step < PPBAR)
    {
        s->patterns[pattern_num][step].event_type = MIDI_ON;
    }
}

void seq_rm_micro_hit(sequencer *s, int pattern_num, int step)
{
    // printf("Rm'ing %d\n", step);
    if (seq_is_valid_pattern_num(s, pattern_num) && step < PPBAR)
    {
        midi_event_clear(&s->patterns[pattern_num][step]);
    }
}

void seq_swing_pattern(sequencer *s, int pattern_num, int swing_setting)
{
    if (seq_is_valid_pattern_num(s, pattern_num))
    {

        int old_swing_setting = s->pattern_num_swing_setting[pattern_num];
        if (old_swing_setting == swing_setting)
            return;

        // printf("Setting pattern %d swing setting to %d\n", pattern_num,
        //       swing_setting);
        s->pattern_num_swing_setting[pattern_num] = swing_setting;

        int hitz_to_swing[64] = {
            0}; // arbitrary - shouldn't have more than 64 hits
        int hitz_to_swing_idx = 0;

        int multi = PPBAR / s->pattern_len;

        int idx = multi; // miss out first sixteenth
        while (idx < PPBAR)
        {
            int next_idx = idx + multi;
            // printf("IDX %d // next idx %d\n", idx, next_idx);
            for (; idx < next_idx; idx++)
            {
                if (s->patterns[s->cur_pattern][idx].event_type == MIDI_ON)
                {
                    // printf("Found a hit at %d\n", idx);
                    hitz_to_swing[hitz_to_swing_idx++] = idx;
                }
            }
            idx += multi;
        }

        int diff = (swing_setting - old_swing_setting) *
                   4;            // swing moves in incs of 4%
        int num_ppz = multi * 2; // i.e. percent between two increments
        int pulses_to_move = num_ppz / 100.0 * diff;
        // printf("Pulses to move is %d\n", pulses_to_move);

        for (int i = 0; i < 64; i++)
        {
            if (hitz_to_swing[i] == 0)
                break;
            int hit = hitz_to_swing[i];
            seq_mv_micro_hit(s, pattern_num, hit, hit + pulses_to_move);
        }
    }
}

void seq_set_sloppiness(sequencer *s, int sloppy_setting)
{
    s->sloppiness = sloppy_setting;
}

midi_event *seq_get_pattern(sequencer *s, int pattern_num)
{
    if (seq_is_valid_pattern_num(s, pattern_num))
        return s->patterns[pattern_num];
    else
        return NULL;
}

void seq_set_pattern(sequencer *s, int pattern_num, midi_event *pattern)
{
    if (seq_is_valid_pattern_num(s, pattern_num))
    {
        clear_pattern(s->patterns[pattern_num]);
        for (int i = 0; i < PPBAR; ++i)
            s->patterns[pattern_num][i] = pattern[i];
    }
}

bool seq_set_num_patterns(sequencer *s, int num_patterns)
{
    if (num_patterns > 0 && num_patterns < 20)
    {
        s->num_patterns = num_patterns;
        return true;
    }
    return false;
}
