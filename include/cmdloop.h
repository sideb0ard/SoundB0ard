#pragma once

#include <stdbool.h>

#include <memory>

#include <lo/lo.h>

#include "audioutils.h"
#include "digisynth.h"
#include "dxsynth.h"
#include "minisynth.h"

void *loopy();

int exxit(void);
int parse_wurds_from_cmd(char wurds[][SIZE_OF_WURD], char *line);
bool parse_minisynth_settings_change(MiniSynth *ms, char wurds[][SIZE_OF_WURD]);
bool parse_dxsynth_settings_change(dxsynth *ms, char wurds[][SIZE_OF_WURD]);
bool parse_digisynth_settings_change(digisynth *ms, char wurds[][SIZE_OF_WURD]);

bool is_valid_file(char *filename);
