#pragma once

#include <stdbool.h>
#include <wchar.h>

#include "digisynth_voice.h"
#include "sound_generator.h"
#include "synthbase.h"

typedef struct digisynth
{
    SOUNDGEN sound_generator;
    synthbase base;

    digisynth_voice *m_voices[MAX_VOICES];

} digisynth;

digisynth *new_digisynth(void);

// sound generator interface //////////////
double digisynth_gennext(void *self);
// void digisynth_gennext(void* self, double* frame_vals, int framesPerBuffer);
void digisynth_status(void *self, wchar_t *status_string);
void digisynth_setvol(void *self, double v);
double digisynth_getvol(void *self);
void digisynth_sg_start(void *self);
void digisynth_sg_stop(void *self);
void digisynth_del_self(void *self);

////////////////////////////////////

// bool digisynth_prepare_for_play(digisynth *synth);
// void digisynth_stop(digisynth *ms);
// void digisynth_update(digisynth *synth);

// void minisynth_handle_midi_note(minisynth *ms, int note, int velocity,
//                                bool update_last_midi);
// bool minisynth_midi_note_on(minisynth *self, unsigned int midinote,
//                            unsigned int velocity);
// bool minisynth_midi_note_off(minisynth *self, unsigned int midinote,
//                             unsigned int velocity, bool all_notes_off);
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
