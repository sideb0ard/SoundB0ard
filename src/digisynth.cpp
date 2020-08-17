#include <stdlib.h>
#include <string.h>

#include "digisynth.h"
#include "midi_freq_table.h"
#include "mixer.h"
#include "utils.h"

#include <iostream>

extern mixer *mixr;

DigiSynth::DigiSynth(std::string sample_path)
{
    std::cout << "NEW DIGI SYNTH! - " << sample_path << " \n ";

    sample_path_ = sample_path;

    type = DIGISYNTH_TYPE;

    for (int i = 0; i < MAX_VOICES; i++)
    {
        digisynth_voice_init(&m_voices[i], sample_path);
    }

    m_last_note_frequency = -1.0;

    active = true;
}

stereo_val DigiSynth::genNext()
{
    if (!active)
        return (stereo_val){0, 0};

    double accum_out_left = 0.0;
    double accum_out_right = 0.0;

    double out_left = 0.0;
    double out_right = 0.0;

    for (int i = 0; i < MAX_VOICES; i++)
    {
        digisynth_voice_gennext(&m_voices[i], &out_left, &out_right);
        accum_out_left += out_left;
        accum_out_right += out_right;
    }

    pan = fmin(pan, 1.0);
    pan = fmax(pan, -1.0);
    double pan_left = 0.707;
    double pan_right = 0.707;
    calculate_pan_values(pan, &pan_left, &pan_right);

    stereo_val return_val = {.left = accum_out_left * volume * pan_left,
                             .right = accum_out_right * volume * pan_right};

    // return_val = effector(this, return_val);
    return return_val;
}

std::string DigiSynth::Info()
{
    std::stringstream ss;

    // swprintf(status_string, MAX_STATIC_STRING_SZ,
    //         WANSI_COLOR_WHITE "%s" WCOOL_COLOR_YELLOW
    //                           " vol:%.2f pan:%.2f active:%s midi_note_1:%d "
    //                           "midi_note_2:%d midi_note_3:%d "
    //                           "sample_len:%d read_idx:%d",
    //         audiofile, volume, pan, active ? "true" : "false",
    //         engine.midi_note_1, engine.midi_note_2, engine.midi_note_3,
    //         m_voices[0].m_osc1.afd.samplecount,
    //         m_voices[0].m_osc1.m_read_idx);
    // wchar_t scratch[1024] = {};
    // sequence_engine_status(&engine, scratch);
    // wcscat(status_string, scratch);
    return ss.str();
}

std::string DigiSynth::Status()
{
    std::stringstream ss;
    ss << "TODO";
    return ss.str();
}

void DigiSynth::start()
{
    active = true;
    engine.cur_step = mixr->timing_info.sixteenth_note_tick % 16;
}
void DigiSynth::stop()
{
    active = false;
    noteOff({});
}

void digisynth_load_wav(DigiSynth *ds, std::string filename)
{
    ds->sample_path_ = filename;
    for (int i = 0; i < MAX_VOICES; i++)
    {
        audiofile_data_import_file_contents(&ds->m_voices[i].m_osc1.afd,
                                            filename);
        ;
    }
}

void DigiSynth::noteOn(midi_event ev)
{
    unsigned int midinote = ev.data1;
    unsigned int velocity = ev.data2;
    for (int i = 0; i < MAX_VOICES; i++)
    {
        digisynth_voice *dsv = &m_voices[i];

        if (!dsv->m_voice.m_note_on)
        {
            voice_note_on(&dsv->m_voice, midinote, velocity,
                          get_midi_freq(midinote), m_last_note_frequency);
            break;
        }
    }
}

void DigiSynth::noteOff(midi_event ev)
{
    for (int i = 0; i < MAX_VOICES; i++)
    {
        voice_note_off(&m_voices[i].m_voice, ev.data1);
    }
}

void digisynth_update(DigiSynth *ds) { (void)ds; }
void DigiSynth::SetParam(std::string name, double val) {}
double DigiSynth::GetParam(std::string name) { return 0; }
