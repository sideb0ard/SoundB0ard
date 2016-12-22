#pragma once

#define SIZE_OF_WURD 21 // 20 char word plus terminator
#define NUM_WURDS 25

void loopy(void);
void interpret(char *line);

int  exxit(void);
int  parse_wurds_from_cmd(char wurds[][SIZE_OF_WURD], char *line);
void char_array_to_seq_string_pattern(char *dest_pattern,
                                      char char_array[NUM_WURDS][SIZE_OF_WURD],
                                      int start, int end);
bool is_valid_soundgen_num(int soundgen_num);
bool is_valid_file(char *filename);
