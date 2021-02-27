#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

#include "mixer.h"
#include "sequenceengine.h"
#include "utils.h"
#include <drumsampler.h>
#include <drumsynth.h>
#include <pattern_utils.h>

extern const int key_midi_mapping[NUM_KEYS];
extern const char *key_names[NUM_KEYS];
extern const compat_key_list compat_keys[NUM_KEYS];

const char *s_arp_mode[] = {"UP", "DOWN", "UPDOWN", "RAND"};
const char *s_arp_speed[] = {"32", "24", "16", "12", "8", "6", "4", "3"};

extern mixer *mixr;

void sequence_engine_add_temporal_event(sequenceengine *engine, int midi_tick,
                                        midi_event ev)
{
    midi_pattern_add_event(engine->temporal_events, midi_tick, ev);
}

bool sequence_engine_list_presets(unsigned int synthtype)
{
    std::cout << "IT ME!\n";

    FILE *presetzzz = NULL;
    switch (synthtype)
    {
    case (MINISYNTH_TYPE):
        presetzzz = fopen(MOOG_PRESET_FILENAME, "r+");
        break;
    case (DXSYNTH_TYPE):
        presetzzz = fopen(DX_PRESET_FILENAME, "r+");
        break;
    }

    if (presetzzz == NULL)
        return false;

    char line[256];
    char setting_key[512];
    char setting_val[512];

    char *tok, *last_tok;
    char const *sep = "::";

    while (fgets(line, sizeof(line), presetzzz))
    {
        for (tok = strtok_r(line, sep, &last_tok); tok;
             tok = strtok_r(NULL, sep, &last_tok))
        {
            sscanf(tok, "%[^=]=%s", setting_key, setting_val);
            if (strcmp(setting_key, "name") == 0)
            {
                printf("%s\n", setting_val);
                break;
            }
        }
    }

    fclose(presetzzz);

    return true;
}

void sequence_engine_clear_events_from(sequenceengine *self, int start_idx)
{
    midi_pattern_clear_events_from(self->temporal_events, start_idx);
}
