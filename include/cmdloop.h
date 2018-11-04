#pragma once

#include "audioutils.h"
#include "digisynth.h"
#include "dxsynth.h"
#include "minisynth.h"
#include "step_sequencer.h"
#include <stdbool.h>

void *loopy(void *arg);
void interpret(char *line);

int stacksize(void);

int exxit(void);
int parse_wurds_from_cmd(char wurds[][SIZE_OF_WURD], char *line);
bool parse_step_sequencer_command(int soundgen_num, int target_pattern_num,
                                  char wurds[][SIZE_OF_WURD], int num_wurds);
bool parse_minisynth_settings_change(minisynth *ms, char wurds[][SIZE_OF_WURD]);
bool parse_dxsynth_settings_change(dxsynth *ms, char wurds[][SIZE_OF_WURD]);
bool parse_digisynth_settings_change(digisynth *ms, char wurds[][SIZE_OF_WURD]);
void char_array_to_seq_string_pattern(step_sequencer *s, char *dest_pattern,
                                      char char_array[NUM_WURDS][SIZE_OF_WURD],
                                      int start, int end);
// void char_pattern_to_midi_pattern(sequence_engine *base, int dest_pattern,
//                                  char char_array[NUM_WURDS][SIZE_OF_WURD],
//                                  int start, int end);
// bool extract_chord_from_char_notation(char *wurd, int *tick,
//                                      chord_midi_notes *chnotes);

bool is_valid_file(char *filename);
