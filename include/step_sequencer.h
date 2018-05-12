#pragma once

#include <stdbool.h>
#include <wchar.h>

#include "defjams.h"

#define MAX_SEQUENCER_PATTERNS 10

typedef struct step_sequencer
{
    int sixteenth_tick;
    int midi_tick;

    int pattern_len; // in musical steps, i.e. 24th or 16th

    midi_pattern patterns[MAX_SEQUENCER_PATTERNS];
    double pattern_position_amp[MAX_SEQUENCER_PATTERNS][PPBAR];
    int pattern_num_loops[MAX_SEQUENCER_PATTERNS];
    int pattern_num_swing_setting[MAX_SEQUENCER_PATTERNS];
    midi_pattern backup_pattern_while_getting_crazy; // store current pattern so
                                                     // algorithms can use slot
    int num_patterns;
    int cur_pattern;
    int cur_pattern_iteration;
    bool multi_pattern_mode;

    bool generate_en;
    int generate_mode;
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

} step_sequencer;

void seq_init(step_sequencer *s);
bool seq_tick(step_sequencer *s);
void seq_status(step_sequencer *s, wchar_t *status_string);

void seq_set_sample_amp(step_sequencer *s, int pattern_num, int pattern_position,
                        double v);
void seq_set_sample_amp_from_char_pattern(step_sequencer *s, int pattern_num,
                                          char *amp_pattern);
void seq_set_random_sample_amp(step_sequencer *s, int pattern_num);
void add_char_pattern(step_sequencer *s, char *pattern);
void change_char_pattern(step_sequencer *s, int pattern_num, char *pattern);

void seq_set_multi_pattern_mode(step_sequencer *s, bool multi);
void seq_change_num_loops(step_sequencer *s, int pattern_num, int num_loops);

void pattern_char_to_pattern(step_sequencer *s, char *char_pattern,
                             midi_event *final_pattern);
void wchar_version_of_amp(step_sequencer *s, int pattern_num, wchar_t apattern[49]);

void seq_set_randamp(step_sequencer *s, bool on);
void seq_set_pattern_len(step_sequencer *s, int len);

void seq_set_generate_mode(step_sequencer *s, unsigned int mode);
void seq_set_generate_enable(step_sequencer *s, bool b);
void seq_set_generate_src(step_sequencer *s, int generate_src);

void seq_clear_pattern(step_sequencer *s, int pattern_num);
void seq_set_backup_mode(step_sequencer *s, bool on);
void seq_set_max_generations(step_sequencer *s, int max);

void seq_wchar_binary_version_of_pattern(step_sequencer *s, midi_pattern p,
                                         wchar_t *bin_num);
void seq_char_binary_version_of_pattern(step_sequencer *s, midi_pattern p,
                                        char *bin_num);
void seq_set_gridsteps(step_sequencer *s, unsigned int gridsteps);
void seq_print_pattern(step_sequencer *s, unsigned int pattern_num);
bool seq_is_valid_pattern_num(step_sequencer *s, int pattern_num);
void seq_mv_hit(step_sequencer *s, int pattern_num, int stepfrom,
                int stepto); // these three deal with hits in 16ths
void seq_add_hit(step_sequencer *s, int pattern_num, int step); // above
void seq_rm_hit(step_sequencer *s, int pattern_num, int step);  // above
void seq_mv_micro_hit(step_sequencer *s, int pattern_num, int stepfrom,
                      int stepto); // these deal with hits in midi pulses
void seq_add_micro_hit(step_sequencer *s, int pattern_num, int step);
void seq_rm_micro_hit(step_sequencer *s, int pattern_num, int step);
void seq_swing_pattern(step_sequencer *s, int pattern_num, int swing_setting);

void seq_set_sloppiness(step_sequencer *s, int sloppy_setting);
int sloppy_weight(step_sequencer *s, int position);

midi_event *seq_get_pattern(step_sequencer *s, int pattern_num);
void seq_set_pattern(step_sequencer *s, int pattern_num, midi_event *pattern);
bool seq_is_valid_pattern(step_sequencer *s, int pattern_num);
bool seq_set_num_patterns(step_sequencer *s, int num_patterns);
