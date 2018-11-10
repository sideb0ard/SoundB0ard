#pragma once

#include "audioutils.h"
#include "digisynth.h"
#include "dxsynth.h"
#include "minisynth.h"
#include <stdbool.h>

void *loopy(void *arg);
void interpret(char *line);

int stacksize(void);

int exxit(void);
int parse_wurds_from_cmd(char wurds[][SIZE_OF_WURD], char *line);
bool parse_minisynth_settings_change(minisynth *ms, char wurds[][SIZE_OF_WURD]);
bool parse_dxsynth_settings_change(dxsynth *ms, char wurds[][SIZE_OF_WURD]);
bool parse_digisynth_settings_change(digisynth *ms, char wurds[][SIZE_OF_WURD]);

bool is_valid_file(char *filename);
