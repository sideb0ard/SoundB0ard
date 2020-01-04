#pragma once

#include <stdbool.h>
#include <wchar.h>

#include "SoundGenerator.h"
#include "digisynth_voice.h"

class digisynth : public SoundGenerator
{
  public:
    digisynth(char *filename);
    ~digisynth() {}
    stereo_val genNext() override;
    void status(wchar_t *wstring) override;
    void start() override;
    void stop() override;
    void noteOn(midi_event ev) override;
    void noteOff(midi_event ev) override;
    void SetParam(std::string name, double val) override;
    double GetParam(std::string name) override;

  public:
    char audiofile[1024];
    digisynth_voice m_voices[MAX_VOICES];

    double m_last_note_frequency;
};

void digisynth_load_wav(digisynth *ds, char *filename);

void digisynth_update(digisynth *ds);

////////////////////////////////////

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
