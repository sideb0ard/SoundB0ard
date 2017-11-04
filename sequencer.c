#include <libgen.h>
#include <math.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "defjams.h"
#include "mixer.h"
#include "sequencer.h"
#include "sequencer_utils.h"
#include "utils.h"

extern mixer *mixr;
extern wchar_t *sparkchars;

const double DEFAULT_AMP = 0.7;

const char *s_markov_mode[] = {"boombap", "haus", "snare"};

void seq_init(sequencer *seq)
{

    seq->sixteenth_tick =
        0; // TODO - this is less relevant, now that i also have 24th
    seq->midi_tick = 0;

    memset(seq->matrix1, 0, sizeof seq->matrix1);
    memset(seq->matrix2, 0, sizeof seq->matrix2);

    seq->num_patterns = 0;
    seq->cur_pattern = 0;
    seq->multi_pattern_mode = true;
    seq->cur_pattern_iteration = 1;
    seq->gridsteps = SIXTEENTH;
    seq->pattern_len = 16;

    for (int i = 0; i < NUM_SEQUENCER_PATTERNS; i++)
    {
        seq->pattern_num_loops[i] = 1;
        for (int j = 0; j < PPBAR; j++)
        {
            seq->pattern_position_amp[i][j] = DEFAULT_AMP;
            seq->patterns[i][j] = 0;
            seq->backup_pattern_while_getting_crazy[j] = 0;
        }
    }

    seq->game_of_life_on = false;
    seq->life_generation = 0;
    seq->life_every_n_loops = 0;

    seq->euclidean_on = false;
    seq->euclidean_generation = 0;
    seq->euclidean_every_n_loops = 0;

    seq->randamp_on = false;
    seq->randamp_generation = 0;
    seq->randamp_every_n_loops = 0;

    seq->shuffle_on = false;
    seq->shuffle_generation = 0;
    seq->shuffle_every_n_loops = 0;

    seq->markov_on = false;
    seq->markov_mode = MARKOVBOOMBAP;
    seq->markov_generation = 0;
    seq->markov_every_n_loops = 0;

    seq->bitwise_on = false;
    seq->bitwise_mode = 0;
    seq->bitwise_generation = 0;
    seq->bitwise_counter = 1044;
    seq->bitwise_every_n_loops = 0;

    seq->sloppiness = 0;
    seq->max_generation = 0;
}

bool seq_tick(sequencer *seq)
{
    if (mixr->timing_info.sixteenth_note_tick != seq->sixteenth_tick)
    {
        seq->sixteenth_tick = mixr->timing_info.sixteenth_note_tick;

        if (seq->sixteenth_tick % 4 == 0)
        {
            if (seq->shuffle_on)
            {
                if (seq->shuffle_every_n_loops > 0)
                {
                    if (seq->shuffle_generation % seq->shuffle_every_n_loops ==
                        0)
                    {
                        seq_set_backup_mode(seq, true);
                        next_shuffle_generation(seq);
                    }
                    else
                    {
                        seq_set_backup_mode(seq, false);
                    }
                }
                else if (seq->max_generation > 0)
                {
                    if (seq->shuffle_generation >= seq->max_generation)
                    {
                        seq->shuffle_generation = 0;
                        seq_set_shuffle(seq, false);
                    }
                }
                else
                {
                    next_shuffle_generation(seq);
                }
                seq->shuffle_generation++;
            }
        }

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

            if (seq->game_of_life_on)
            {
                if (seq->life_every_n_loops > 0)
                {
                    if (seq->life_generation % seq->life_every_n_loops == 0)
                    {
                        seq_set_backup_mode(seq, true);
                        next_life_generation(seq);
                    }
                    else
                    {
                        seq_set_backup_mode(seq, false);
                    }
                }
                else if (seq->max_generation > 0)
                {
                    if (seq->life_generation >= seq->max_generation)
                    {
                        seq->life_generation = 0;
                        seq_set_game_of_life(seq, false);
                    }
                }
                else
                {
                    next_life_generation(seq);
                }
                seq->life_generation++;
            }
            else if (seq->euclidean_on)
            {
                if (seq->euclidean_every_n_loops > 0)
                {
                    if (seq->euclidean_generation %
                            seq->euclidean_every_n_loops ==
                        0)
                    {
                        seq_set_backup_mode(seq, true);
                        next_euclidean_generation(seq, 0);
                    }
                    else
                    {
                        seq_set_backup_mode(seq, false);
                    }
                }
                else if (seq->max_generation > 0)
                {
                    if (seq->euclidean_generation >= seq->max_generation)
                    {
                        seq->euclidean_generation = 0;
                        seq_set_euclidean(seq, false);
                    }
                }
                else
                {
                    next_euclidean_generation(seq, 0);
                }
                seq->euclidean_generation++;
            }
            else if (seq->markov_on)
            {
                if (seq->markov_every_n_loops > 0)
                {
                    if (seq->markov_generation % seq->markov_every_n_loops == 0)
                    {
                        seq_set_backup_mode(seq, true);
                        next_markov_generation(seq);
                    }
                    else
                    {
                        seq_set_backup_mode(seq, false);
                    }
                }
                else if (seq->max_generation > 0)
                {
                    if (seq->markov_generation >= seq->max_generation)
                    {
                        seq->markov_generation = 0;
                        seq_set_markov(seq, false);
                    }
                }
                else
                {
                    next_markov_generation(seq);
                }
                seq->markov_generation++;
            }
            else if (seq->bitwise_on)
            {
                bool gen_new_pattern = false;
                if (seq->bitwise_every_n_loops > 0)
                {
                    if (seq->bitwise_generation % seq->bitwise_every_n_loops ==
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
                else if (seq->max_generation > 0 &&
                         seq->bitwise_generation > seq->max_generation)
                {
                    seq->bitwise_generation = 0;
                    seq_set_bitwise(seq, false);
                }
                else
                {
                    gen_new_pattern = true;
                }

                if (gen_new_pattern)
                {
                    int bit_pattern = gimme_a_bitwise_short(
                        seq->bitwise_mode, seq->bitwise_counter);

                    memset(&seq->patterns[seq->cur_pattern], 0,
                           PPBAR * sizeof(int));
                    convert_bitshift_pattern_to_pattern(
                        bit_pattern, (int *)&seq->patterns[seq->cur_pattern],
                        PPBAR, seq->gridsteps);

                    seq->bitwise_counter++;
                }
                seq->bitwise_generation++;
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
                             int final_pattern[PPBAR])
{
    int sp_count = 0;
    char *sp, *sp_last;
    int pattern[s->pattern_len];
    char const *sep = " ";

    printf("GOtz char_pattern %s\n", char_pattern);

    // extract numbers from string into pattern
    for (sp = strtok_r(char_pattern, sep, &sp_last); sp;
         sp = strtok_r(NULL, sep, &sp_last))
    {
        pattern[sp_count++] = atoi(sp);
    }
    memset(final_pattern, 0, PPBAR * sizeof(int));
    int mult = 0;
    switch (s->gridsteps)
    {
    case (SIXTEENTH):
        mult = PPSIXTEENTH;
        break;
    case (TWENTYFOURTH):
        mult = PPTWENTYFOURTH;
        break;
    }

    for (int i = 0; i < sp_count; i++)
    {
        printf("Adding hit to midi step %d\n", pattern[i] * mult);
        final_pattern[pattern[i] * mult] = 1;
    }
}

// game of life algo helpers
void int_to_matrix(int pattern, int matrix[GRIDWIDTH][GRIDWIDTH])
{
    int row = 0;
    for (int i = 0, p = 1; i < INTEGER_LENGTH; i++, p *= 2)
    {

        if (i != 0 && (i % GRIDWIDTH == 0))
            row++;

        int col = i % GRIDWIDTH;
        // printf("POS %d %d\n", row, col);
        if (pattern & p)
        {
            matrix[row][col] = 1;
        }
    }
}

int matrix_to_int(int matrix[GRIDWIDTH][GRIDWIDTH])
{
    int return_pattern = 0;

    int row = 0;
    for (int i = 0, p = 1; i < SEQUENCER_PATTERN_LEN; i++, p *= 2)
    {

        if (i != 0 && (i % GRIDWIDTH == 0))
            row++;
        int col = i % GRIDWIDTH;

        if (matrix[row][col] == 1)
        {
            return_pattern = return_pattern | p;
        }
    }

    return return_pattern;
}

void next_euclidean_generation(sequencer *s, int pattern_num)
{
    memset(&s->patterns[pattern_num], 0, PPBAR * sizeof(int));
    int rand_steps = (rand() % 9) + 1;
    int bitpattern = create_euclidean_rhythm(rand_steps, s->pattern_len);
    if (rand() % 2)
        bitpattern =
            shift_bits_to_leftmost_position(bitpattern, s->pattern_len);
    if (rand() % 2) // mask first half
        bitpattern = (bitpattern | 65280) - 65280;

    convert_bitshift_pattern_to_pattern(
        bitpattern, (int *)&s->patterns[pattern_num], PPBAR, s->gridsteps);
}

// game of life algo
void next_life_generation(sequencer *s)
{
    memset(s->matrix1, 0, sizeof s->matrix1);
    memset(s->matrix2, 0, sizeof s->matrix2);
    int cur_pattern_as_int =
        pattern_as_int_representation((int *)&s->patterns[s->cur_pattern]);
    // printf("CUR PATTERN AS INT %d\n", cur_pattern_as_int);
    int_to_matrix(cur_pattern_as_int, s->matrix1);

    for (int y = 0; y < GRIDWIDTH; y++)
    {
        for (int x = 0; x < GRIDWIDTH; x++)
        {
            int neighbors = 0;

            // printf("My co-ords y:%d x:%d\n", y, x);
            for (int rel_y = y - 1; rel_y <= y + 1; rel_y++)
            {
                for (int rel_x = x - 1; rel_x <= x + 1; rel_x++)
                {
                    int n_y = rel_y;
                    int n_x = rel_x;
                    if (n_y < 0)
                        n_y += GRIDWIDTH;
                    if (n_y == GRIDWIDTH)
                        n_y -= GRIDWIDTH;
                    if (n_x < 0)
                        n_x += GRIDWIDTH;
                    if (n_x == GRIDWIDTH)
                        n_x -= GRIDWIDTH;
                    if (!(n_x == x && n_y == y))
                    {
                        // printf("My neighbs y:%d x:%d val
                        // - %d\n", n_y, n_x);
                        if (s->matrix1[n_y][n_x] == 1)
                            neighbors += 1;
                    }
                }
            }
            // printf("[%d][%d] - I gots %d neighbors\n", y, x,
            // neighbors);

            // the RULES
            if (s->matrix1[y][x] == 0 && neighbors == 3)
                s->matrix2[y][x] = 1;

            if (s->matrix1[y][x] == 1 && (neighbors == 2 || neighbors == 3))
                s->matrix2[y][x] = 1;

            if (s->matrix1[y][x] == 1 && (neighbors > 3 || neighbors < 2))
                s->matrix2[y][x] = 0;
        }
    }

    int new_pattern = matrix_to_int(s->matrix2);
    // printf("NEW PATTERN! %d\n", new_pattern);
    if (new_pattern == 0)
        new_pattern = seed_pattern();

    memset(&s->patterns[0], 0, PPBAR * sizeof(int));
    convert_bitshift_pattern_to_pattern(new_pattern, s->patterns[0], PPBAR,
                                        SIXTEENTH);
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

void next_shuffle_generation(sequencer *s)
{
    int positions_to_shuffle[100] = {0};
    int positions_to_shuffle_idx = 0;
    for (int i = 0; i < PPBAR; i++)
    {
        if (s->patterns[s->cur_pattern][i])
        {
            positions_to_shuffle[positions_to_shuffle_idx++] = i;
            s->patterns[s->cur_pattern][i] = 0;
        }
    }

    if (positions_to_shuffle_idx > 0)
    {
        for (int i = 0; i < positions_to_shuffle_idx; ++i)
        {
            int newposition =
                (positions_to_shuffle[i] + PPSIXTEENTH * 3) % PPBAR;
            s->patterns[s->cur_pattern][newposition] = 1;
        }
    }
}

void next_markov_generation(sequencer *s)
{
    memset(&s->patterns[0], 0, PPBAR * sizeof(int));

    if (rand() % 10 > 7)
    {
        memset(s->patterns[s->cur_pattern], 0, PPBAR * sizeof(int));
        return;
    }

    for (int i = 0; i < s->pattern_len; i++)
    {
        int randy = rand() % 100;
        int randyTripleOrDouble = rand() % 100;

        int position = i * PPSIXTEENTH;

        if (s->markov_mode == MARKOVHAUS)
        {
            switch (i)
            {
            case 0:
                if (randy < 95)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 1:
                if (randy < 5)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 2:
                if (randy < 5)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 3:
                if (randy < 3)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 4:
                if (randy < 95)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 5:
                if (randy < 5)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 6:
                if (randy < 5)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 7:
                if (randy < 2)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 8:
                if (randy < 95)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 9:
                if (randy < 5)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 10:
                if (randy < 5)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 11:
                if (randy < 2)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 12:
                if (randy < 95)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 13:
                if (randy < 5)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 14:
                if (randy < 25)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 15:
                if (randy < 2)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            }
        }
        else if (s->markov_mode == MARKOVBOOMBAP)
        {
            if (mixr->debug_mode)
                printf("88 y'all!!\n");
            switch (i)
            {
            case 0:
                if (randy < 95)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 1:
                if (randy < 5)
                {
                    int sloppy_position = sloppy_weight(s, position);
                    s->patterns[s->cur_pattern][sloppy_position] = 1;
                }
                break;
            case 2:
                if (randy < 25)
                {
                    int sloppy_position = sloppy_weight(s, position);
                    s->patterns[s->cur_pattern][sloppy_position] = 1;
                }
                break;
            case 3:
                if (randy < 35)
                {
                    int sloppy_position = sloppy_weight(s, position);
                    s->patterns[s->cur_pattern][sloppy_position] = 1;
                }
                break;
            case 4:
                if (randy < 15)
                {
                    if (randyTripleOrDouble > 90)
                    {
                        s->patterns[s->cur_pattern][position] = 1;
                        s->patterns[s->cur_pattern][position + PPTWENTYFOURTH] =
                            1;
                        if (randyTripleOrDouble > 95)
                        {
                            s->patterns[s->cur_pattern]
                                       [position + (PPTWENTYFOURTH * 2)] = 1;
                        }
                        i += 4; // skip next few beats
                    }
                    else
                    {
                        int sloppy_position = sloppy_weight(s, position);
                        s->patterns[s->cur_pattern][sloppy_position] = 1;
                    }
                }
                break;
            case 5:
                if (randy < 3)
                {
                    int sloppy_position = sloppy_weight(s, position);
                    s->patterns[s->cur_pattern][sloppy_position] = 1;
                }
                break;
            case 6:
                if (randy < 15)
                {
                    int sloppy_position = sloppy_weight(s, position);
                    s->patterns[s->cur_pattern][sloppy_position] = 1;
                }
                break;
            case 7:
                if (randy < 12)
                {
                    int sloppy_position = sloppy_weight(s, position);
                    s->patterns[s->cur_pattern][sloppy_position] = 1;
                }
                break;
            case 8:
                if (randy < 95)
                {
                    if (randyTripleOrDouble > 80)
                    {
                        s->patterns[s->cur_pattern][position] = 1;
                        s->patterns[s->cur_pattern][position + PPTWENTYFOURTH] =
                            1;
                        if (randyTripleOrDouble > 95)
                        {
                            s->patterns[s->cur_pattern]
                                       [position + (PPTWENTYFOURTH * 2)] = 1;
                        }
                        i += 4; // skip next few beats
                    }
                    else
                    {
                        int sloppy_position = sloppy_weight(s, position);
                        s->patterns[s->cur_pattern][sloppy_position] = 1;
                    }
                }
                break;
            case 9:
                if (randy < 3)
                {
                    int sloppy_position = sloppy_weight(s, position);
                    s->patterns[s->cur_pattern][sloppy_position] = 1;
                }
                break;
            case 10:
                if (randy < 5)
                {
                    int sloppy_position = sloppy_weight(s, position);
                    s->patterns[s->cur_pattern][sloppy_position] = 1;
                }
                break;
            case 11:
                if (randy < 25)
                {
                    int sloppy_position = sloppy_weight(s, position);
                    s->patterns[s->cur_pattern][sloppy_position] = 1;
                }
                break;
            case 12:
                if (randy < 5)
                {
                    if (randyTripleOrDouble > 90)
                    {
                        s->patterns[s->cur_pattern][position] = 1;
                        s->patterns[s->cur_pattern][position + PPTWENTYFOURTH] =
                            1;
                        if (randyTripleOrDouble > 95)
                        {
                            s->patterns[s->cur_pattern]
                                       [position + (PPTWENTYFOURTH * 2)] = 1;
                        }
                        i += 4; // skip next few beats
                    }
                    else
                    {
                        int sloppy_position = sloppy_weight(s, position);
                        s->patterns[s->cur_pattern][sloppy_position] = 1;
                    }
                }
                break;
            case 13:
                if (randy < 5)
                {
                    int sloppy_position = sloppy_weight(s, position);
                    s->patterns[s->cur_pattern][sloppy_position] = 1;
                }
                break;
            case 14:
                if (randy < 5)
                {
                    if (randyTripleOrDouble > 90)
                    {
                        s->patterns[s->cur_pattern][position] = 1;
                        s->patterns[s->cur_pattern][position + PPTWENTYFOURTH] =
                            1;
                        i += 4; // skip next few beats
                    }
                    else
                    {
                        int sloppy_position = sloppy_weight(s, position);
                        s->patterns[s->cur_pattern][sloppy_position] = 1;
                    }
                }
                break;
            case 15:
                if (randy < 2)
                {
                    int sloppy_position = sloppy_weight(s, position);
                    s->patterns[s->cur_pattern][sloppy_position] = 1;
                }
                break;
            }
        }
        else if (s->markov_mode == MARKOVSNARE)
        {
            switch (i)
            {
            case 2:
                if (randy < 4)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 3:
                if (randy < 3)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 4:
                if (randy < 90)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 9:
                if (randy < 5)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 10:
                if (randy < 7)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 12:
                if (randyTripleOrDouble > 90)
                {
                    s->patterns[s->cur_pattern][position] = 1;
                    s->patterns[s->cur_pattern][position + PPTWENTYFOURTH] = 1;
                    if (randyTripleOrDouble > 95)
                    {
                        s->patterns[s->cur_pattern]
                                   [position + (PPTWENTYFOURTH * 2)] = 1;
                    }
                    i += 4; // skip next few beats
                }
                else if (randy < 90)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 13:
                if (randyTripleOrDouble > 90)
                {
                    s->patterns[s->cur_pattern][position] = 1;
                    s->patterns[s->cur_pattern][position + PPTWENTYFOURTH] = 1;
                    if (randyTripleOrDouble > 95)
                    {
                        s->patterns[s->cur_pattern]
                                   [position + (PPTWENTYFOURTH * 2)] = 1;
                    }
                    i += 4; // skip next few beats
                }
                else if (randy < 10)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 14:
                if (randy < 10)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            case 15:
                if (randy < 10)
                    s->patterns[s->cur_pattern][position] = 1;
                break;
            }
        }
    }
}

// void update_pattern(void *self, int newpattern)
// {
//     sequencer *seq = self;
//     seq->pattern = newpattern;
// }

void seq_status(sequencer *seq, wchar_t *status_string)
{
    swprintf(
        status_string, MAX_PS_STRING_SZ,
        L"\n      CurStep: %d life_mode: %d Every_n: %d Pattern Len: %d "
        "markov: %d markov_mode: %s(%d) Markov_Every_n: %d Multi: %d Max Gen: "
        "%d\n      Bitwise: %d Bitwise mode:%d Bitwise_every_n: %d Euclidean: "
        "%d Euclid_n: %d"
        "sloppy: %d shuffle_on: %d shuffle_every_n: %d",
        seq->cur_pattern, seq->game_of_life_on, seq->life_every_n_loops,
        seq->pattern_len, seq->markov_on, s_markov_mode[seq->markov_mode],
        seq->markov_mode, seq->markov_every_n_loops, seq->multi_pattern_mode,
        seq->max_generation, seq->bitwise_on, seq->bitwise_mode,
        seq->bitwise_every_n_loops, seq->euclidean_on,
        seq->euclidean_every_n_loops, seq->sloppiness, seq->shuffle_on,
        seq->shuffle_every_n_loops);
    wchar_t pattern_details[128];
    char spattern[seq->pattern_len + 1];
    wchar_t apattern[seq->pattern_len + 1];
    for (int i = 0; i < seq->num_patterns; i++)
    {
        seq_char_binary_version_of_pattern(seq, seq->patterns[i], spattern);
        wchar_version_of_amp(seq, i, apattern);
        swprintf(pattern_details, 127,
                 L"\n      [%d] - [%s] %ls  numloops: %d Swing: %d", i,
                 spattern, apattern, seq->pattern_num_loops[i],
                 seq->pattern_num_swing_setting[i]);
        wcscat(status_string, pattern_details);
    }
    wcscat(status_string, WANSI_COLOR_RESET);
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

// void add_int_pattern(sequencer *s, int pattern)
//{
//    s->patterns[s->num_patterns++] = pattern;
//    s->cur_pattern++;
//}

void change_char_pattern(sequencer *s, int pattern_num, char *pattern)
{
    pattern_char_to_pattern(s, pattern, s->patterns[pattern_num]);
}

// void change_int_pattern(sequencer *s, int pattern_num, int pattern)
// {
//     s->patterns[pattern_num] = pattern;
// }

int seed_pattern()
{
    int pattern = 0;
    for (int i = 0; i < SEQUENCER_PATTERN_LEN; i++)
    {
        int randy = rand() % 100;
        if (randy > 50)
        {
            pattern = pattern | (1 << i);
        }
    }
    return pattern;
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

void seq_set_euclidean(sequencer *s, bool b)
{
    s->euclidean_generation = 0;
    s->euclidean_on = b;
}

void seq_set_game_of_life(sequencer *s, bool b)
{
    s->life_generation = 0;
    s->game_of_life_on = b;
    seq_set_backup_mode(s, b);
}

void seq_set_markov(sequencer *s, bool b)
{
    s->markov_generation = 0;
    s->markov_on = b;
    seq_set_backup_mode(s, b);
}

void seq_set_bitwise(sequencer *s, bool b)
{
    s->bitwise_generation = 0;
    s->bitwise_on = b;
    seq_set_backup_mode(s, b);
}

void seq_set_backup_mode(sequencer *s, bool on)
{
    if (on)
    {
        memset(s->backup_pattern_while_getting_crazy, 0, PPBAR * sizeof(int));
        memcpy(s->backup_pattern_while_getting_crazy, &s->patterns[0],
               PPBAR * sizeof(int));
        // memset(&s->patterns[0], 0, PPBAR * sizeof(int));
        s->multi_pattern_mode = false;
        s->cur_pattern = 0;
    }
    else
    {
        memset(&s->patterns[0], 0, PPBAR * sizeof(int));
        memcpy(&s->patterns[0], s->backup_pattern_while_getting_crazy,
               PPBAR * sizeof(int));
        s->multi_pattern_mode = true;
    }
}

void seq_set_max_generations(sequencer *s, int max) { s->max_generation = max; }

void seq_set_markov_mode(sequencer *s, unsigned int mode)
{
    s->markov_mode = mode;
}

void seq_set_bitwise_mode(sequencer *s, unsigned int mode)
{
    printf("Setting BITWISE mode\n");
    s->bitwise_mode = mode;
}

void seq_set_randamp(sequencer *s, bool b) { s->randamp_on = b; }

void seq_set_shuffle(sequencer *s, bool b) { s->shuffle_on = b; }

void seq_wchar_binary_version_of_pattern(sequencer *s, seq_pattern p,
                                         wchar_t *bin_num)
{
    for (int i = 0; i < s->pattern_len; i++)
    {
        if (is_int_member_in_array(1, &p[i * PPSIXTEENTH], PPSIXTEENTH))
            bin_num[i] = sparkchars[5];
        else
            bin_num[i] = sparkchars[0];
    }
    bin_num[s->pattern_len] = '\0';
}

void seq_char_binary_version_of_pattern(sequencer *s, seq_pattern p,
                                        char *bin_num)
{
    int incs = 0;
    switch (s->pattern_len)
    {
    case (24):
        incs = PPTWENTYFOURTH;
        break;
    case (16):
        incs = PPSIXTEENTH;
        break;
    }
    for (int i = 0; i < s->pattern_len; i++)
    {
        if (is_int_member_in_array(1, &p[i * incs], incs))
            bin_num[i] = '1';
        else
            bin_num[i] = '0';
    }
    bin_num[s->pattern_len] = '\0';
}

void seq_set_gridsteps(sequencer *s, unsigned int gridsteps)
{
    printf("SEQ! changing gridsteps to %d\n", gridsteps);
    switch (gridsteps)
    {
    case (16):
        s->gridsteps = SIXTEENTH;
        s->pattern_len = gridsteps;
        break;
    case (24):
        s->gridsteps = TWENTYFOURTH;
        s->pattern_len = gridsteps;
        break;
    }
}

void seq_print_pattern(sequencer *s, unsigned int pattern_num)
{
    if (seq_is_valid_pattern_num(s, pattern_num))
    {
        printf("Pattern %d\n", pattern_num);
        for (int i = 0; i < PPBAR; i++)
        {
            if (s->patterns[pattern_num][i] == 1)
            {
                printf("[%d] on\n", i);
            }
        }
    }
}

bool seq_is_valid_pattern_num(sequencer *d, int pattern_num)
{
    if (pattern_num >= 0 && pattern_num < d->num_patterns)
    {
        return true;
    }
    return false;
}

void seq_mv_hit(sequencer *s, int pattern_num, int stepfrom, int stepto)
{
    int multi = 0;
    if (s->pattern_len == 16)
    {
        multi = PPSIXTEENTH;
    }
    else if (s->pattern_len == 24)
    {
        multi = PPTWENTYFOURTH;
    }

    int mstepfrom = stepfrom * multi;
    int mstepto = stepto * multi;

    seq_mv_micro_hit(s, pattern_num, mstepfrom, mstepto);
}

void seq_add_hit(sequencer *s, int pattern_num, int step)
{
    int multi = 0;
    if (s->pattern_len == 16)
    {
        multi = PPSIXTEENTH;
    }
    else if (s->pattern_len == 24)
    {
        multi = PPTWENTYFOURTH;
    }

    int mstep = step * multi;
    seq_add_micro_hit(s, pattern_num, mstep);
}

void seq_rm_hit(sequencer *s, int pattern_num, int step)
{
    int multi = 0;
    if (s->pattern_len == 16)
    {
        multi = PPSIXTEENTH;
    }
    else if (s->pattern_len == 24)
    {
        multi = PPTWENTYFOURTH;
    }

    int mstep = step * multi;
    seq_rm_micro_hit(s, pattern_num, mstep);
}

void seq_mv_micro_hit(sequencer *s, int pattern_num, int stepfrom, int stepto)
{
    printf("SEQ mv micro\n");
    if (seq_is_valid_pattern_num(s, pattern_num))
    {
        if (s->patterns[pattern_num][stepfrom] == 1 && stepto < PPBAR)
        {
            s->patterns[pattern_num][stepfrom] = 0;
            s->patterns[pattern_num][stepto] = 1;
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
    printf("Add'ing %d\n", step);
    if (seq_is_valid_pattern_num(s, pattern_num) && step < PPBAR)
    {
        s->patterns[pattern_num][step] = 1;
    }
}

void seq_rm_micro_hit(sequencer *s, int pattern_num, int step)
{
    printf("Rm'ing %d\n", step);
    if (seq_is_valid_pattern_num(s, pattern_num) && step < PPBAR)
    {
        s->patterns[pattern_num][step] = 0;
    }
}

void seq_swing_pattern(sequencer *s, int pattern_num, int swing_setting)
{
    if (seq_is_valid_pattern_num(s, pattern_num))
    {

        int old_swing_setting = s->pattern_num_swing_setting[pattern_num];
        if (old_swing_setting == swing_setting)
            return;

        printf("Setting pattern %d swing setting to %d\n", pattern_num,
               swing_setting);
        s->pattern_num_swing_setting[pattern_num] = swing_setting;

        int hitz_to_swing[64] = {
            0}; // arbitrary - shouldn't have more than 64 hits
        int hitz_to_swing_idx = 0;

        int multi = 0;
        switch (s->gridsteps)
        {
        case (SIXTEENTH):
            multi = PPSIXTEENTH;
            break;
        case (TWENTYFOURTH):
            multi = PPTWENTYFOURTH;
            break;
        }

        int idx = multi; // miss out first sixteenth
        while (idx < PPBAR)
        {
            int next_idx = idx + multi;
            printf("IDX %d // next idx %d\n", idx, next_idx);
            for (; idx < next_idx; idx++)
            {
                if (s->patterns[s->cur_pattern][idx] == 1)
                {
                    printf("Found a hit at %d\n", idx);
                    hitz_to_swing[hitz_to_swing_idx++] = idx;
                }
            }
            idx += multi;
        }

        int diff = (swing_setting - old_swing_setting) *
                   4;            // swing moves in incs of 4%
        int num_ppz = multi * 2; // i.e. percent between two increments
        int pulses_to_move = num_ppz / 100.0 * diff;
        printf("Pulses to move is %d\n", pulses_to_move);

        for (int i = 0; i < 64; i++)
        {
            if (hitz_to_swing[i] == 0)
                break;
            int hit = hitz_to_swing[i];
            printf("Going to swing hit %d\n", hit);
            seq_mv_micro_hit(s, pattern_num, hit, hit + pulses_to_move);
        }
    }
}

void seq_set_sloppiness(sequencer *s, int sloppy_setting)
{
    s->sloppiness = sloppy_setting;
}
