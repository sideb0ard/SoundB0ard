#include "digisynth.h"
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

    synthbase_init(&ds->base);

    strncpy(ds->audiofile, filename, 1023);

    ds->vol = 1.0;

    ds->sound_generator.gennext = &digisynth_gennext;
    ds->sound_generator.status = &digisynth_status;
    ds->sound_generator.setvol = &digisynth_setvol;
    ds->sound_generator.getvol = &digisynth_getvol;
    ds->sound_generator.start = &digisynth_sg_start;
    ds->sound_generator.stop = &digisynth_sg_stop;
    ds->sound_generator.get_num_tracks = &synthbase_get_num_tracks;
    ds->sound_generator.make_active_track = &synthbase_make_active_track;
    ds->sound_generator.type = DIGISYNTH_TYPE;
    ds->sound_generator.active = true;

    for (int i = 0; i < MAX_VOICES; i++)
    {
        digisynth_voice_init(&ds->m_voices[i], filename);
    }

    return ds;
}

// sound generator interface //////////////
double digisynth_gennext(void *self)
{
    (void)self;
    return 0.2;
}

void digisynth_status(void *self, wchar_t *status_string)
{
    printf("DIGI STATUS!\n");
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
