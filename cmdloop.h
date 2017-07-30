#pragma once
#include "looper.h"
#include "minisynth.h"
#include "sequencer.h"
#include <stdbool.h>

#define SIZE_OF_WURD 41 // 40 char word plus terminator
#define NUM_WURDS 25

void print_prompt(void);
void loopy(void);
void interpret(char *line);

int exxit(void);
int parse_wurds_from_cmd(char wurds[][SIZE_OF_WURD], char *line);
void parse_sequencer_command(sequencer *seq, char wurds[][SIZE_OF_WURD],
                             int num_wurds, char *pattern);
bool parse_minisynth_settings_change(minisynth *ms, char wurds[][SIZE_OF_WURD]);
void char_array_to_seq_string_pattern(sequencer *s, char *dest_pattern,
                                      char char_array[NUM_WURDS][SIZE_OF_WURD],
                                      int start, int end);
void char_melody_to_midi_melody(minisynth *ms, int dest_melody,
                                char char_array[NUM_WURDS][SIZE_OF_WURD],
                                int start, int end);
// bool is_valid_soundgen_num(int soundgen_num);
bool is_valid_sample_num(looper *s, int sample_num);
bool is_valid_fx_num(int soundgen_num, int fx_num);
bool is_valid_file(char *filename);
