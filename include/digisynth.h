#pragma once

#include <stdbool.h>
#include <wchar.h>

#include "digisynth_voice.h"
#include "sound_generator.h"
#include "synthbase.h"

typedef struct digisynth
{
    soundgenerator sound_generator;
    synthbase base;

    char audiofile[1024];
    digisynth_voice m_voices[MAX_VOICES];

    double vol;
    double m_last_note_frequency;

} digisynth;

digisynth *new_digisynth(char *filename);

// sound generator interface //////////////
stereo_val digisynth_gennext(void *self);
void digisynth_status(void *self, wchar_t *status_string);
void digisynth_setvol(void *self, double v);
double digisynth_getvol(void *self);
void digisynth_stop(digisynth *d);
void digisynth_sg_start(void *self);
void digisynth_sg_stop(void *self);
void digisynth_del_self(void *self);
int digisynth_get_num_patterns(void *self);
void digisynth_set_num_patterns(void *self, int num_patterns);
void digisynth_make_active_track(void *self, int tracknum);
midi_event *digisynth_get_pattern(void *self, int pattern_num);
void digisynth_set_pattern(void *self, int pattern_num, midi_event *pattern);
bool digisynth_is_valid_pattern(void *self, int pattern_num);

////////////////////////////////////

bool digisynth_midi_note_on(digisynth *self, unsigned int midinote,
                            unsigned int velocity);
bool digisynth_midi_note_off(digisynth *self, unsigned int midinote,
                             unsigned int velocity, bool all_notes_off);
// void minisynth_toggle_delay_mode(minisynth *ms);
//
// void minisynth_print_settings(minisynth *ms);
// bool minisynth_save_settings(minisynth *ms, char *preset_name);
// bool minisynth_load_settings(minisynth *ms, char *preset_name);
////bool minisynth_list_presets(void);
////bool minisynth_check_if_preset_exists(char *preset_to_find);
//
// void minisynth_set_vol(minisynth *ms, double val);
// void minisynth_set_reset_to_zero(minisynth *ms, unsigned int val);
