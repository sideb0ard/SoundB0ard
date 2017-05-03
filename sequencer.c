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

void seq_init(sequencer *seq)
{

    seq->sixteenth_tick = 0;
    seq->midi_tick = 0;

    memset(seq->matrix1, 0, sizeof seq->matrix1);
    memset(seq->matrix2, 0, sizeof seq->matrix2);

    seq->num_patterns = 0;
    seq->cur_pattern = 0;
    seq->multi_pattern_mode = true;
    seq->cur_pattern_iteration = 1;

    for (int i = 0; i < NUM_SEQUENCER_PATTERNS; i++) {
        seq->pattern_num_loops[i] = 1;
        for (int j = 0; j < PPBAR; j++) {
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

    seq->markov_on = false;
    seq->markov_mode = MARKOVBOOMBAP;
    seq->markov_generation = 0;
    seq->markov_every_n_loops = 0;

    seq->bitwise_on = false;
    seq->bitwise_mode = 0;
    seq->bitwise_generation = 0;
    seq->bitwise_every_n_loops = 0;

    seq->max_generation = 0;
}

bool seq_tick(sequencer *seq)
{
    if (mixr->sixteenth_note_tick != seq->sixteenth_tick) {
        seq->sixteenth_tick = mixr->sixteenth_note_tick;

        if (seq->sixteenth_tick % 16 == 0) {

            if (seq->multi_pattern_mode) {
                seq->cur_pattern_iteration--;
                if (seq->cur_pattern_iteration == 0) {
                    seq->cur_pattern =
                        (seq->cur_pattern + 1) % seq->num_patterns;
                    seq->cur_pattern_iteration =
                        seq->pattern_num_loops[seq->cur_pattern];
                }
            }

            if (seq->game_of_life_on) {
                if (seq->life_every_n_loops > 0) {
                    if (seq->life_generation % seq->life_every_n_loops == 0) {
                        seq_set_backup_mode(seq, true);
                        //next_life_generation(seq);
                    }
                    else {
                        seq_set_backup_mode(seq, false);
                    }
                }
                else if (seq->max_generation > 0) {
                    if (seq->life_generation >= seq->max_generation) {
                        seq->life_generation = 0;
                        seq_set_game_of_life(seq, false);
                    }
                }
                else {
                    next_life_generation(seq);
                }
                seq->life_generation++;
            }
            else if (seq->euclidean_on) {
                if (seq->euclidean_every_n_loops > 0) {
                    if (seq->euclidean_generation % seq->euclidean_every_n_loops ==
                        0) {
                        seq_set_backup_mode(seq, true);
                        next_euclidean_generation(seq);
                    }
                    else {
                        seq_set_backup_mode(seq, false);
                    }
                }
                else if (seq->max_generation > 0) {
                    if (seq->euclidean_generation >= seq->max_generation) {
                        seq->euclidean_generation = 0;
                        seq_set_euclidean(seq, false);
                    }
                }
                else {
                    next_euclidean_generation(seq);
                }
                seq->euclidean_generation++;
            }
            else if (seq->markov_on) {
                if (seq->markov_every_n_loops > 0) {
                    if (seq->markov_generation % seq->markov_every_n_loops ==
                        0) {
                        seq_set_backup_mode(seq, true);
                        next_markov_generation(seq);
                    }
                    else {
                        seq_set_backup_mode(seq, false);
                    }
                }
                else if (seq->max_generation > 0) {
                    if (seq->markov_generation >= seq->max_generation) {
                        seq->markov_generation = 0;
                        seq_set_markov(seq, false);
                    }
                }
                else {
                    next_markov_generation(seq);
                }
                seq->markov_generation++;
            }
            //else if (seq->bitwise_on) {
            //    if (seq->bitwise_every_n_loops > 0) {
            //        if (seq->bitwise_generation % seq->bitwise_every_n_loops ==
            //            0) {
            //            seq_set_backup_mode(seq, true);
            //            int new_pattern = gimme_a_bitwise_int(seq->bitwise_mode,
            //                                                  mixr->cur_sample);
            //            seq->patterns[seq->cur_pattern] = new_pattern;
            //        }
            //        else {
            //            seq_set_backup_mode(seq, false);
            //        }
            //    }
            //    else if (seq->max_generation > 0) {
            //        if (seq->bitwise_generation >= seq->max_generation) {
            //            seq->bitwise_generation = 0;
            //            seq_set_bitwise(seq, false);
            //        }
            //    }
            //    else {
            //        int new_pattern = gimme_a_bitwise_int(seq->bitwise_mode,
            //                                              mixr->cur_sample);
            //        seq->patterns[seq->cur_pattern] = new_pattern;
            //    }
            //    seq->bitwise_generation++;
            //}
            if (seq->randamp_on) {
                if (seq->randamp_every_n_loops > 0) {
                    if (seq->randamp_generation % seq->randamp_every_n_loops ==
                        0) {
                        seq_set_random_sample_amp(seq, seq->cur_pattern);
                    }
                }
                else {
                    seq_set_random_sample_amp(seq, seq->cur_pattern);
                }
                seq->randamp_generation++;
            }
        }
        return true;
    }
    return false;
}

void int_pattern_to_array(int pattern, int *pat_array)
{
    for (int i = 0, p = 1; p < 65535; i++, p *= 2) {
        if (pattern & p) {
            pat_array[i] = 1;
        }
        else {
            pat_array[i] = 0;
        }
    }
}

void pattern_char_to_pattern(char *char_pattern, int final_pattern[PPBAR])
{
    int sp_count = 0;
    char *sp, *sp_last;
    int pattern[16];
    char const *sep = " ";

    // extract numbers from string into pattern
    for (sp = strtok_r(char_pattern, sep, &sp_last); sp;
         sp = strtok_r(NULL, sep, &sp_last)) {
        pattern[sp_count++] = atoi(sp);
    }
    for (int i = 0; i < sp_count; i++) {
        final_pattern[pattern[i]*PPSIXTEENTH] = 1;
    }
}


// game of life algo helpers
void int_to_matrix(int pattern, int matrix[GRIDWIDTH][GRIDWIDTH])
{
    int row = 0;
    for (int i = 0, p = 1; i < INTEGER_LENGTH; i++, p *= 2) {

        if (i != 0 && (i % GRIDWIDTH == 0))
            row++;

        int col = i % GRIDWIDTH;
        // printf("POS %d %d\n", row, col);
        if (pattern & p) {
            matrix[row][col] = 1;
        }
    }
}

int matrix_to_int(int matrix[GRIDWIDTH][GRIDWIDTH])
{
    int return_pattern = 0;

    int row = 0;
    for (int i = 0, p = 1; i < SEQUENCER_PATTERN_LEN; i++, p *= 2) {

        if (i != 0 && (i % GRIDWIDTH == 0))
            row++;
        int col = i % GRIDWIDTH;

        if (matrix[row][col] == 1) {
            return_pattern = return_pattern | p;
        }
    }

    return return_pattern;
}

void next_euclidean_generation(sequencer *s)
{
    int rand_steps = (rand() % 9) + 1;
    int bitpattern = create_euclidean_rhythm(rand_steps, SEQUENCER_PATTERN_LEN);
    if (rand() % 2 == 1)
        bitpattern = shift_bits_to_leftmost_position(bitpattern, SEQUENCER_PATTERN_LEN);
    convert_bitshift_pattern_to_pattern(bitpattern, (int*) &s->patterns[s->cur_pattern], PPBAR, SIXTEENTH);
}

// game of life algo
void next_life_generation(sequencer *self)
{
    memset(self->matrix1, 0, sizeof self->matrix1);
    memset(self->matrix2, 0, sizeof self->matrix2);
    //int_to_matrix(self->patterns[self->cur_pattern], self->matrix1);

    for (int y = 0; y < GRIDWIDTH; y++) {
        for (int x = 0; x < GRIDWIDTH; x++) {

            int neighbors = 0;

            printf("My co-ords y:%d x:%d\n", y, x);
            for (int rel_y = y - 1; rel_y <= y + 1; rel_y++) {
                for (int rel_x = x - 1; rel_x <= x + 1; rel_x++) {
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
                    if (!(n_x == x && n_y == y)) {
                        // printf("My neighbs y:%d x:%d val - %d\n", n_y, n_x);
                        if (self->matrix1[n_y][n_x] == 1)
                            neighbors += 1;
                    }
                }
            }
            printf("[%d][%d] - I gots %d neighbors\n", y, x, neighbors);

            // the RULES
            if (self->matrix1[y][x] == 0 && neighbors == 3)
                self->matrix2[y][x] = 1;

            if (self->matrix1[y][x] == 1 && (neighbors == 2 || neighbors == 3))
                self->matrix2[y][x] = 1;

            if (self->matrix1[y][x] == 1 && (neighbors > 3 || neighbors < 2))
                self->matrix2[y][x] = 0;
        }
    }

    // int return_pattern = matrix_to_int(self->matrix2);
    //int new_pattern = matrix_to_int(self->matrix2);
    //// printf("NEW PATTERN! %d\n", new_pattern);
    //if (new_pattern == 0)
    //    new_pattern = seed_pattern();
    //self->patterns[self->cur_pattern] = new_pattern;
}

void next_markov_generation(sequencer *d)
{
    if (rand() % 10 > 8) {
        memset(d->patterns[d->cur_pattern], 0, PPBAR*sizeof(int));
        return;
    }

    for (int i = 0; i < 16; i++)
    {
        int randy = rand() % 100;
        if (mixr->debug_mode)
            printf("Iiii %d Randy! %d\n", i, randy);

        if (d->markov_mode == MARKOVHAUS) {
            if (mixr->debug_mode)
                printf("HAUS MUSIC ALL NIGHT LONG!\n");

            switch (i) {
            case 0:
                if (randy < 95) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 1:
                if (randy < 5) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 2:
                if (randy < 5) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 3:
                if (randy < 3) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 4:
                if (randy < 95) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 5:
                if (randy < 5) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 6:
                if (randy < 5) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 7:
                if (randy < 2) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 8:
                if (randy < 95) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 9:
                if (randy < 5) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 10:
                if (randy < 5) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 11:
                if (randy < 2) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 12:
                if (randy < 95) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 13:
                if (randy < 5) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 14:
                if (randy < 25) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 15:
                if (randy < 2) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            }
        }
        else if (d->markov_mode == MARKOVBOOMBAP) {
            if (mixr->debug_mode)
                printf("88 y'all!!\n");
            switch (i) {
            case 0:
                if (randy < 95) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 1:
                if (randy < 5) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 2:
                if (randy < 25) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 3:
                if (randy < 35) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 4:
                if (randy < 15) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 5:
                if (randy < 5) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 6:
                if (randy < 25) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 7:
                if (randy < 12) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 8:
                if (randy < 95) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 9:
                if (randy < 5) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 10:
                if (randy < 5) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 11:
                if (randy < 25) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 12:
                if (randy < 5) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 13:
                if (randy < 5) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 14:
                if (randy < 25) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
                break;
            case 15:
                if (randy < 2) d->patterns[d->cur_pattern][i*PPSIXTEENTH] = 1;
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
        L"\n      CurStep: %d life_mode: %d Every_n: %d "
        "markov_on: %d markov_mode: %s Markov_Every_n: %d Multi: %d Max Gen: %d"
        L"\n      Bitwise: %d Bitwise_every_n: %d Euclidean: %d Euclid_n: %d",
        seq->cur_pattern, seq->game_of_life_on, seq->life_every_n_loops,
        seq->markov_on, seq->markov_mode ? "boombap" : "haus",
        seq->markov_every_n_loops, seq->multi_pattern_mode, seq->max_generation,
        seq->bitwise_on, seq->bitwise_every_n_loops, seq->euclidean_on, seq->euclidean_every_n_loops);
    wchar_t pattern_details[128];
    char spattern[17];
    wchar_t apattern[17];
    for (int i = 0; i < seq->num_patterns; i++) {
        char_binary_version_of_pattern(seq->patterns[i], spattern);
        wchar_version_of_amp(seq, i, apattern);
        swprintf(pattern_details, 127, L"\n      [%d] - [%s] %ls  numloops: %d",
                 i, spattern, apattern, seq->pattern_num_loops[i]);
        wcscat(status_string, pattern_details);
    }
    wcscat(status_string, WANSI_COLOR_RESET);
}

// TODO - fix this - being lazy and want it finished NOW!
void wchar_version_of_amp(sequencer *d, int pattern_num, wchar_t apattern[17])
{
    for (int i = 0; i < SEQUENCER_PATTERN_LEN; i++) {
        double amp = d->pattern_position_amp[pattern_num][i];
        int idx = (int)floor(scaleybum(0, 1.1, 0, wcslen(sparkchars), amp));
        apattern[i] = sparkchars[idx];
        // wprintf(L"\n%ls\n", sparkchars[3]);
    }
    apattern[16] = '\0';
}

void add_char_pattern(sequencer *s, char *pattern)
{
    pattern_char_to_pattern(pattern, s->patterns[s->num_patterns++]);
    s->cur_pattern++;
}

//void add_int_pattern(sequencer *s, int pattern)
//{
//    s->patterns[s->num_patterns++] = pattern;
//    s->cur_pattern++;
//}

void change_char_pattern(sequencer *s, int pattern_num, char *pattern)
{
    pattern_char_to_pattern(pattern, s->patterns[pattern_num]);
}

// void change_int_pattern(sequencer *s, int pattern_num, int pattern)
// {
//     s->patterns[pattern_num] = pattern;
// }

int seed_pattern()
{
    int pattern = 0;
    for (int i = 0; i < SEQUENCER_PATTERN_LEN; i++) {
        int randy = rand() % 100;
        if (randy > 50) {
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
    for (int i = 0; i < SEQUENCER_PATTERN_LEN; i++) {
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
         sp = strtok_r(NULL, sep, &sp_last)) {
        spattern[sp_count++] = sp;
    }

    for (int i = 0; i < sp_count; i++) {
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
    if (pattern_num < s->num_patterns && num_loops > 0) {
        s->pattern_num_loops[pattern_num] = num_loops;
    }
}

void seq_set_euclidean(sequencer *s, bool b)
{
    s->euclidean_generation = 0;
    if (b) {
        s->euclidean_on = true;
        //seq_set_backup_mode(s, b);
    }
    else {
        s->euclidean_on = false;
        //seq_set_backup_mode(s, b);
    }
}
void seq_set_game_of_life(sequencer *s, bool b)
{
    s->life_generation = 0;
    if (b) {
        s->game_of_life_on = true;
        seq_set_backup_mode(s, b);
    }
    else {
        s->game_of_life_on = false;
        seq_set_backup_mode(s, b);
    }
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
    if (b) {
        s->bitwise_on = true;
        seq_set_backup_mode(s, b);
    }
    else {
        s->bitwise_on = false;
        seq_set_backup_mode(s, b);
    }
}

void seq_set_backup_mode(sequencer *s, bool on)
{
    if (on) {
        // printf("EUCLID ON!\n");
        // print_pattern(s->backup_pattern_while_getting_crazy, PPBAR);
        memset(s->backup_pattern_while_getting_crazy, 0, PPBAR*sizeof(int));
        memcpy(s->backup_pattern_while_getting_crazy, &s->patterns[0], PPBAR*sizeof(int));
        memset(&s->patterns[0], 0, PPBAR*sizeof(int));
        s->multi_pattern_mode = false;
        s->cur_pattern = 0;
        // print_pattern(s->backup_pattern_while_getting_crazy, PPBAR);
        // printf("\n");
    }
    else {
        //printf("EUCLID OFF!\n");
        memset(&s->patterns[0], 0, PPBAR*sizeof(int));
        memcpy(&s->patterns[0], s->backup_pattern_while_getting_crazy, PPBAR*sizeof(int));
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
    s->bitwise_mode = mode;
}

void seq_set_randamp(sequencer *s, bool b)
{
    s->randamp_on = b;
}
