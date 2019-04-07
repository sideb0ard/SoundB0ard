#include <stdlib.h>
#include <string.h>

#include "digisynth.h"
#include "midi_freq_table.h"
#include "mixer.h"
#include "utils.h"

extern mixer *mixr;

digisynth *new_digisynth(char *filename)
{
    printf("NEW DIGI SYNTH!\n");

    digisynth *ds = (digisynth *)calloc(1, sizeof(digisynth));
    if (ds == NULL)
    {
        printf("Barfy\n");
        return NULL;
    }

    sequence_engine_init(&ds->engine, (void *)ds, DIGISYNTH_TYPE);

    strncpy(ds->audiofile, filename, 1023);

    ds->sg.gennext = &digisynth_gennext;
    ds->sg.status = &digisynth_status;
    ds->sg.set_volume = &sound_generator_set_volume;
    ds->sg.get_volume = &sound_generator_get_volume;
    ds->sg.set_pan = &sound_generator_set_pan;
    ds->sg.get_pan = &sound_generator_get_pan;
    ds->sg.start = &digisynth_sg_start;
    ds->sg.stop = &digisynth_sg_stop;
    ds->sg.event_notify = &sequence_engine_event_notify;
    ds->sg.self_destruct = &digisynth_del_self;
    ds->sg.type = DIGISYNTH_TYPE;
    ds->sg.active = true;
    sound_generator_init(&ds->sg);

    for (int i = 0; i < MAX_VOICES; i++)
    {
        digisynth_voice_init(&ds->m_voices[i], filename);
    }

    ds->m_last_note_frequency = -1.0;

    return ds;
}

void digisynth_load_wav(digisynth *ds, char *filename)
{
    strncpy(ds->audiofile, filename, 1023);
    for (int i = 0; i < MAX_VOICES; i++)
    {
        audiofile_data_import_file_contents(&ds->m_voices[i].m_osc1.afd,
                                            filename);
        ;
    }
}

// sound generator interface //////////////
stereo_val digisynth_gennext(void *self)
{
    digisynth *ds = (digisynth *)self;

    if (!ds->sg.active)
        return (stereo_val){0, 0};

    double accum_out_left = 0.0;
    double accum_out_right = 0.0;

    double out_left = 0.0;
    double out_right = 0.0;

    for (int i = 0; i < MAX_VOICES; i++)
    {
        digisynth_voice_gennext(&ds->m_voices[i], &out_left, &out_right);
        accum_out_left += out_left;
        accum_out_right += out_right;
    }

    ds->sg.pan = fmin(ds->sg.pan, 1.0);
    ds->sg.pan = fmax(ds->sg.pan, -1.0);
    double pan_left = 0.707;
    double pan_right = 0.707;
    calculate_pan_values(ds->sg.pan, &pan_left, &pan_right);

    stereo_val return_val = {.left = accum_out_left * ds->sg.volume * pan_left,
                             .right =
                                 accum_out_right * ds->sg.volume * pan_right};

    return_val = effector(&ds->sg, return_val);
    return return_val;
}

void digisynth_status(void *self, wchar_t *status_string)
{
    digisynth *ds = (digisynth *)self;
    swprintf(status_string, MAX_STATIC_STRING_SZ,
             WANSI_COLOR_WHITE "%s" WCOOL_COLOR_YELLOW
                               " vol:%.2f pan:%.2f active:%s midi_note_1:%d "
                               "midi_note_2:%d midi_note_3:%d "
                               "sample_len:%d read_idx:%d",
             ds->audiofile, ds->sg.volume, ds->sg.pan,
             ds->sg.active ? "true" : "false", ds->engine.midi_note_1,
             ds->engine.midi_note_2, ds->engine.midi_note_3,
             ds->m_voices[0].m_osc1.afd.samplecount,
             ds->m_voices[0].m_osc1.m_read_idx);
    wchar_t scratch[1024] = {};
    sequence_engine_status(&ds->engine, scratch);
    wcscat(status_string, scratch);
}

void digisynth_sg_start(void *self)
{
    digisynth *ds = (digisynth *)self;
    ds->sg.active = true;
    ds->engine.cur_step = mixr->timing_info.sixteenth_note_tick % 16;
}
void digisynth_sg_stop(void *self)
{
    digisynth *ds = (digisynth *)self;
    ds->sg.active = false;
    digisynth_stop(ds);
}

void digisynth_del_self(void *self)
{
    digisynth *ds = (digisynth *)self;
    free(ds);
}

void digisynth_stop(digisynth *d) { digisynth_midi_note_off(d, 0, 0, true); }
////////////////////////////////////

// bool digisynth_prepare_for_play(digisynth *synth);
// void digisynth_update(digisynth *synth);

// void minisynth_handle_midi_note(minisynth *ms, int note, int velocity,
//                                bool update_last_midi);
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
//
bool digisynth_midi_note_on(digisynth *ds, unsigned int midinote,
                            unsigned int velocity)
{
    for (int i = 0; i < MAX_VOICES; i++)
    {
        digisynth_voice *dsv = &ds->m_voices[i];

        if (!dsv->m_voice.m_note_on)
        {
            voice_note_on(&dsv->m_voice, midinote, velocity,
                          get_midi_freq(midinote), ds->m_last_note_frequency);
            break;
        }
    }

    return true;
}

bool digisynth_midi_note_off(digisynth *ds, unsigned int midinote,
                             unsigned int velocity, bool all_notes_off)
{
    (void)velocity;
    (void)all_notes_off;

    for (int i = 0; i < MAX_VOICES; i++)
    {
        voice_note_off(&ds->m_voices[i].m_voice, midinote);
    }
    return true;
}

void digisynth_update(digisynth *ds) {}
