#include "digisynth.h"
#include "midi_freq_table.h"
#include <stdlib.h>
#include <string.h>

digisynth *new_digisynth(char *filename)
{
    printf("NEW DIGI SYNTH!\n");

    digisynth *ds = (digisynth *)calloc(1, sizeof(digisynth));
    if (ds == NULL)
    {
        printf("Barfy\n");
        return NULL;
    }

    synthbase_init(&ds->base, (void *)ds, DIGISYNTH_TYPE);

    strncpy(ds->audiofile, filename, 1023);

    ds->vol = 1.0;

    ds->sound_generator.gennext = &digisynth_gennext;
    ds->sound_generator.ps_status = &digisynth_status;
    ds->sound_generator.full_status = &digisynth_status;
    ds->sound_generator.setvol = &digisynth_setvol;
    ds->sound_generator.getvol = &digisynth_getvol;
    ds->sound_generator.start = &digisynth_sg_start;
    ds->sound_generator.stop = &digisynth_sg_stop;
    ds->sound_generator.event_notify = &synthbase_event_notify;
    ds->sound_generator.self_destruct = &digisynth_del_self;
    ds->sound_generator.get_num_tracks = &digisynth_get_num_tracks;
    ds->sound_generator.make_active_track = &digisynth_make_active_track;
    ds->sound_generator.type = DIGISYNTH_TYPE;
    ds->sound_generator.active = true;

    for (int i = 0; i < MAX_VOICES; i++)
    {
        digisynth_voice_init(&ds->m_voices[i], filename);
    }

    ds->m_last_note_frequency = -1.0;

    return ds;
}

// sound generator interface //////////////
stereo_val digisynth_gennext(void *self)
{
    digisynth *ds = (digisynth *)self;

    if (!ds->sound_generator.active)
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

    accum_out_left *= ds->vol;
    accum_out_left = effector(&ds->sound_generator, accum_out_left);
    accum_out_left = envelopor(&ds->sound_generator, accum_out_left);

    accum_out_right *= ds->vol;
    accum_out_right = effector(&ds->sound_generator, accum_out_right);
    accum_out_right = envelopor(&ds->sound_generator, accum_out_right);

    return (stereo_val){.left = accum_out_left, .right = accum_out_right};
}

void digisynth_status(void *self, wchar_t *status_string)
{
    digisynth *ds = (digisynth *)self;
    swprintf(status_string, MAX_PS_STRING_SZ,
             L"[DIGISYNTH '%s'] - Vol: %.2f Active: %s\n", ds->audiofile,
             ds->vol, ds->sound_generator.active ? "true" : "false");
    wchar_t scratch[512];
    synthbase_status(&ds->base, scratch);
    wcscat(status_string, scratch);
}

void digisynth_setvol(void *self, double v)
{
    digisynth *ds = (digisynth *)self;
    ds->vol = v;
}

double digisynth_getvol(void *self)
{
    digisynth *ds = (digisynth *)self;
    return ds->vol;
}

int digisynth_get_num_tracks(void *self)
{
    digisynth *ds = (digisynth *)self;
    return synthbase_get_num_tracks(&ds->base);
}

void digisynth_make_active_track(void *self, int tracknum)
{
    digisynth *ds = (digisynth *)self;
    synthbase_make_active_track(&ds->base, tracknum);
}

void digisynth_sg_start(void *self)
{
    digisynth *ds = (digisynth *)self;
    ds->sound_generator.active = true;
}
void digisynth_sg_stop(void *self)
{
    digisynth *ds = (digisynth *)self;
    ds->sound_generator.active = false;
}

void digisynth_del_self(void *self)
{
    digisynth *ds = (digisynth *)self;
    free(ds);
}

////////////////////////////////////

// bool digisynth_prepare_for_play(digisynth *synth);
// void digisynth_stop(digisynth *ms);
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
