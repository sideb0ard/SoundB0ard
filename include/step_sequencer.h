#pragma once

#include <stdbool.h>
#include <wchar.h>

#include "defjams.h"

#define MAX_SEQUENCER_PATTERNS 10

typedef int seq_pattern[PPBAR];

typedef struct sequencer
{
    int sixteenth_tick;
    int midi_tick;

    int pattern_len; // in musical steps, i.e. 24th or 16th

    seq_pattern patterns[MAX_SEQUENCER_PATTERNS];
    double pattern_position_amp[MAX_SEQUENCER_PATTERNS][PPBAR];
    int pattern_num_loops[MAX_SEQUENCER_PATTERNS];
    int pattern_num_swing_setting[MAX_SEQUENCER_PATTERNS];
    seq_pattern backup_pattern_while_getting_crazy; // store current pattern so
                                                    // algorithms can use slot
    int num_patterns;
    int cur_pattern;
    int cur_pattern_iteration;
    bool multi_pattern_mode;

    bool generate_mode;
    int generate_src;
    int generate_generation;
    int generate_max_generation;
    int generate_every_n_loops;

    // randomizes amplitude
    bool randamp_on;
    int randamp_generation;
    int randamp_every_n_loops;

    bool visualize;

    int sloppiness; // 0 - 10

} sequencer;

void seq_init(sequencer *s);
bool seq_tick(sequencer *s);
void seq_status(sequencer *s, wchar_t *status_string);

void seq_set_sample_amp(sequencer *s, int pattern_num, int pattern_position,
                        double v);
void seq_set_sample_amp_from_char_pattern(sequencer *s, int pattern_num,
                                          char *amp_pattern);
void seq_set_random_sample_amp(sequencer *s, int pattern_num);
void add_char_pattern(sequencer *s, char *pattern);
void change_char_pattern(sequencer *s, int pattern_num, char *pattern);

void seq_set_multi_pattern_mode(sequencer *s, bool multi);
void seq_change_num_loops(sequencer *s, int pattern_num, int num_loops);

void pattern_char_to_pattern(sequencer *s, char *char_pattern,
                             int final_pattern[PPBAR]);
void wchar_version_of_amp(sequencer *s, int pattern_num, wchar_t apattern[49]);

void seq_set_randamp(sequencer *s, bool on);
void seq_set_pattern_len(sequencer *s, int len);

void seq_set_generate_mode(sequencer *s, bool b);
void seq_set_generate_src(sequencer *s, int generate_src);

void seq_clear_pattern(sequencer *s, int pattern_num);
void seq_set_backup_mode(sequencer *s, bool on);
void seq_set_max_generations(sequencer *s, int max);

void seq_wchar_binary_version_of_pattern(sequencer *s, seq_pattern p,
                                         wchar_t *bin_num);
void seq_char_binary_version_of_pattern(sequencer *s, seq_pattern p,
                                        char *bin_num);
void seq_set_gridsteps(sequencer *s, unsigned int gridsteps);
void seq_print_pattern(sequencer *s, unsigned int pattern_num);
bool seq_is_valid_pattern_num(sequencer *s, int pattern_num);
void seq_mv_hit(sequencer *s, int pattern_num, int stepfrom,
                int stepto); // these three deal with hits in 16ths
void seq_add_hit(sequencer *s, int pattern_num, int step); // above
void seq_rm_hit(sequencer *s, int pattern_num, int step);  // above
void seq_mv_micro_hit(sequencer *s, int pattern_num, int stepfrom,
                      int stepto); // these deal with hits in midi pulses
void seq_add_micro_hit(sequencer *s, int pattern_num, int step);
void seq_rm_micro_hit(sequencer *s, int pattern_num, int step);
void seq_swing_pattern(sequencer *s, int pattern_num, int swing_setting);

void seq_set_sloppiness(sequencer *s, int sloppy_setting);
int sloppy_weight(sequencer *s, int position);

parceled_pattern seq_get_pattern(sequencer *s, int pattern_num);
void seq_set_pattern(sequencer *s, int pattern_num, parceled_pattern pattern);
bool seq_is_valid_pattern(sequencer *s, int pattern_num);
