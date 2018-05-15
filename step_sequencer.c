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

void step_init(step_sequencer *seq)
{

    seq->sixteenth_tick =
        0; // TODO - this is less relevant, now that i also have 24th
    seq->midi_tick = 0;

    seq->num_patterns = 1;
    seq->cur_pattern = 0;
    seq->multi_pattern_mode = true;
    seq->cur_pattern_iteration = 1;
    seq->pattern_len = 16;

    seq->generate_en = false;
    seq->generate_src = -99;
    seq->generate_generation = 0;
    seq->generate_every_n_loops = 0;
    seq->generate_max_generation = 0;

    for (int i = 0; i < MAX_SEQUENCER_PATTERNS; i++)
        seq->pattern_num_loops[i] = 1;

    seq->sloppiness = 0;

    seq->visualize = false;
}

bool step_tick(step_sequencer *seq)
{
    if (mixr->timing_info.sixteenth_note_tick != seq->sixteenth_tick)
    {
        seq->sixteenth_tick = mixr->timing_info.sixteenth_note_tick;

        if (seq->sixteenth_tick % 16 == 0)
        {

            if (seq->multi_pattern_mode)
            {
                seq->cur_pattern_iteration--;
                if (seq->cur_pattern_iteration == 0)
                {
                    seq->cur_pattern =
                        (seq->cur_pattern + 1) % seq->num_patterns;
                    seq->cur_pattern_iteration =
                        seq->pattern_num_loops[seq->cur_pattern];
                }
            }

            if (seq->generate_en)
            {
                bool gen_new_pattern = false;
                if (seq->generate_every_n_loops > 0)
                {
                    if (seq->generate_generation %
                            seq->generate_every_n_loops ==
                        0)
                    {
                        step_set_backup_mode(seq, true);
                        gen_new_pattern = true;
                    }
                    else
                    {
                        step_set_backup_mode(seq, false);
                    }
                }
                else if (seq->generate_max_generation > 0)
                {
                    if (seq->generate_generation >=
                        seq->generate_max_generation)
                    {
                        seq->generate_generation = 0;
                        step_set_generate_enable(seq, false);
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

                        memset(&seq->patterns[seq->cur_pattern], 0,
                               PPBAR * sizeof(midi_event));

                        int pattern_offset = 0;
                        int division = 1;
                        if (seq->generate_mode == 1)
                            division = 4;
                        for (int i = 0; i < division; i++)
                        {
                            pattern_offset = i * (PPBAR / division);

                            int bit_pattern_len = 16; // default
                            int bit_pattern = sg->generate(
                                (void *)sg, (void *)&bit_pattern_len);

                            if (seq->visualize)
                            {
                                char bit_string[17];
                                char_binary_version_of_short(bit_pattern,
                                                             bit_string);
                                printf("PATTERN: %s\n", bit_string);
                            }

                            convert_bit_pattern_to_midi_pattern(
                                bit_pattern, bit_pattern_len,
                                seq->patterns[seq->cur_pattern], division,
                                pattern_offset);

                            seq->pattern_len = bit_pattern_len;
                        }
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
                        step_set_random_sample_amp(seq, seq->cur_pattern);
                    }
                }
                else
                {
                    step_set_random_sample_amp(seq, seq->cur_pattern);
                }
                seq->randamp_generation++;
            }
        }
        return true;
    }
    return false;
}

void pattern_char_to_pattern(step_sequencer *s, char *char_pattern,
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

int sloppy_weight(step_sequencer *s, int position)
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

void step_status(step_sequencer *seq, wchar_t *status_string)
{
    wchar_t pattern_details[256]; // arbitrary len
    wchar_t patternstr[33] = {0}; // 2 x 16th so that char is wider
    wchar_t apattern[seq->pattern_len + 1];
    for (int i = 0; i < seq->num_patterns; i++)
    {
        pattern_to_string(seq->patterns[i], patternstr);
        swprintf(pattern_details, 255, L"\n[%d]  %ls numloops:%d swing:%d", i,
                 patternstr, seq->pattern_num_loops[i],
                 seq->pattern_num_swing_setting[i]);
        wcscat(status_string, pattern_details);
    }
}

// TODO - fix this - being lazy and want it finished NOW!
void wchar_version_of_amp(step_sequencer *s, int pattern_num, wchar_t *apattern)
{
    for (int i = 0; i < s->pattern_len; i++)
    {
        int v = s->patterns[pattern_num][i].data2;
        int idx = (int)floor(scaleybum(0, 127, 0, wcslen(sparkchars), v));
        apattern[i] = sparkchars[idx];
        // wprintf(L"\n%lc\n", sparkchars[3]);
    }
    apattern[s->pattern_len] = '\0';
}

void add_char_pattern(step_sequencer *s, char *pattern)
{
    pattern_char_to_pattern(s, pattern, s->patterns[s->num_patterns++]);
    s->cur_pattern++;
}

void change_char_pattern(step_sequencer *s, int pattern_num, char *pattern)
{
    pattern_char_to_pattern(s, pattern, s->patterns[pattern_num]);
}

void step_set_sample_velocity(step_sequencer *s, int pattern_num,
                              int pattern_position, unsigned int v)
{
    if (v < 128)
        s->patterns[pattern_num][pattern_position].data2 = v;
    else
        printf("Nae chance, velocity has to be between 0-127\n");
}

void step_set_multi_pattern_mode(step_sequencer *s, bool multi)
{
    s->multi_pattern_mode = multi;
    s->cur_pattern_iteration = s->pattern_num_loops[s->cur_pattern];
}

void step_change_num_loops(step_sequencer *s, int pattern_num, int num_loops)
{
    if (pattern_num < s->num_patterns && num_loops > 0)
    {
        s->pattern_num_loops[pattern_num] = num_loops;
    }
}

void step_set_generate_enable(step_sequencer *s, bool b)
{
    s->generate_generation = 0;
    s->generate_en = b;
    step_set_backup_mode(s, b);
}

void step_clear_pattern(step_sequencer *s, int pattern_num)
{
    memset(&s->patterns[pattern_num], 0, PPBAR * sizeof(midi_event));
}

void step_set_backup_mode(step_sequencer *s, bool on)
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

void step_set_max_generations(step_sequencer *s, int max)
{
    s->generate_max_generation = max;
}

void step_set_generate_src(step_sequencer *s, int src)
{
    s->generate_src = src;
}

void step_set_generate_mode(step_sequencer *s, int unsigned mode)
{
    if (mode < 2)
        s->generate_mode = mode;
}

void step_set_pattern_len(step_sequencer *s, int len)
{
    int ridiculous_size = 101;
    if (len < ridiculous_size)
        s->pattern_len = len;
}
void step_wchar_binary_version_of_pattern(step_sequencer *s, midi_pattern p,
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

void step_char_binary_version_of_pattern(step_sequencer *s, midi_pattern p,
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

void step_print_pattern(step_sequencer *s, unsigned int pattern_num)
{
    if (step_is_valid_pattern_num(s, pattern_num))
    {
        printf("Pattern %d\n", pattern_num);
        for (int i = 0; i < PPBAR; i++)
        {
            if (s->patterns[pattern_num][i].event_type == MIDI_ON)
            {
                printf("[%d] on - velocity = %d\n", i,
                       s->patterns[pattern_num][i].data2);
            }
        }
    }
}

bool step_is_valid_pattern_num(step_sequencer *d, int pattern_num)
{
    if (pattern_num >= 0 && pattern_num < MAX_NUM_MIDI_LOOPS)
    {
        if (pattern_num >= d->num_patterns)
            d->num_patterns = pattern_num + 1;
        return true;
    }
    return false;
}

void step_mv_hit(step_sequencer *s, int pattern_num, int stepfrom, int stepto)
{
    int multi = PPBAR / s->pattern_len;

    int mstepfrom = stepfrom * multi;
    int mstepto = stepto * multi;

    step_mv_micro_hit(s, pattern_num, mstepfrom, mstepto);
}

void step_add_hit(step_sequencer *s, int pattern_num, int step)
{
    int multi = PPBAR / s->pattern_len;
    int mstep = step * multi;
    step_add_micro_hit(s, pattern_num, mstep);
}

void step_rm_hit(step_sequencer *s, int pattern_num, int step)
{
    int multi = PPBAR / s->pattern_len;
    int mstep = step * multi;
    step_rm_micro_hit(s, pattern_num, mstep);
}

void step_mv_micro_hit(step_sequencer *s, int pattern_num, int stepfrom,
                       int stepto)
{
    // printf("SEQ mv micro\n");
    if (step_is_valid_pattern_num(s, pattern_num))
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

void step_add_micro_hit(step_sequencer *s, int pattern_num, int step)
{
    printf("MICRO Add'ing %d\n", step);
    if (step_is_valid_pattern_num(s, pattern_num) && step < PPBAR)
    {
        s->patterns[pattern_num][step].event_type = MIDI_ON;
        s->patterns[pattern_num][step].data2 = DEFAULT_VELOCITY;
    }
}

void step_rm_micro_hit(step_sequencer *s, int pattern_num, int step)
{
    // printf("Rm'ing %d\n", step);
    if (step_is_valid_pattern_num(s, pattern_num) && step < PPBAR)
    {
        midi_event_clear(&s->patterns[pattern_num][step]);
    }
}

void step_swing_pattern(step_sequencer *s, int pattern_num, int swing_setting)
{
    if (step_is_valid_pattern_num(s, pattern_num))
    {

        int old_swing_setting = s->pattern_num_swing_setting[pattern_num];
        if (old_swing_setting == swing_setting)
            return;
        s->pattern_num_swing_setting[pattern_num] = swing_setting;

        int swing_diff = swing_setting - old_swing_setting;

        midi_event new_pattern[PPBAR] = {};
        midi_event *cur_pattern = step_get_pattern(s, pattern_num);
        bool even16th = true;
        for (int i = 0; i < PPBAR; i += PPSIXTEENTH)
        {
            for (int j = 0; j < PPSIXTEENTH; j++)
            {
                int idx = i + j;
                if (cur_pattern[idx].event_type)
                {
                    if (even16th)
                    {
                        // clean copy
                        new_pattern[idx] = cur_pattern[idx];
                    }
                    else
                    {
                        int new_idx =
                            idx + swing_diff * 19; // TODO magic number 19 midi
                                                   // ticks per 4% swing
                        while (new_idx < 0)
                            new_idx = PPBAR - new_idx;
                        while (new_idx >= PPBAR)
                            new_idx = new_idx - PPBAR;
                        new_pattern[new_idx] = cur_pattern[idx];
                    }
                }
            }
            even16th = 1 - even16th;
        }
        step_set_pattern(s, pattern_num, new_pattern);
    }
}

void step_set_sloppiness(step_sequencer *s, int sloppy_setting)
{
    s->sloppiness = sloppy_setting;
}

midi_event *step_get_pattern(step_sequencer *s, int pattern_num)
{
    if (step_is_valid_pattern_num(s, pattern_num))
        return s->patterns[pattern_num];
    else
        return NULL;
}

void step_set_pattern(step_sequencer *s, int pattern_num, midi_event *pattern)
{
    if (step_is_valid_pattern_num(s, pattern_num))
    {
        clear_pattern(s->patterns[pattern_num]);
        for (int i = 0; i < PPBAR; ++i)
            s->patterns[pattern_num][i] = pattern[i];
    }
}

bool step_set_num_patterns(step_sequencer *s, int num_patterns)
{
    if (num_patterns > 0 && num_patterns < 20)
    {
        s->num_patterns = num_patterns;
        return true;
    }
    return false;
}

void step_set_random_sample_amp(step_sequencer *s, int pattern_num)
{
    for (int i = 0; i < PPBAR; i++)
    {
        if (s->patterns[pattern_num][i].event_type == MIDI_ON)
        {
            int randy = rand() % 128;
            step_set_sample_velocity(s, pattern_num, i, randy);
        }
    }
}

void step_set_randamp(step_sequencer *s, bool b) { s->randamp_on = b; }
