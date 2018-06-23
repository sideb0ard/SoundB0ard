#include <portaudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <audioutils.h>
#include <midimaaan.h>
#include <utils.h>

double pa_setup(void)
{
    // PA start me up!
    PaError err;
    err = Pa_Initialize();
    if (err != paNoError)
    {
        printf("Errrrr! couldn't initialize Portaudio: %s\n",
               Pa_GetErrorText(err));
        exit(-1);
    }
    PaStreamParameters params;
    params.device = Pa_GetDefaultOutputDevice();
    if (params.device == paNoDevice)
    {
        printf("BARFd on PA_GetDefaultOutputDevice!\n");
        exit(1);
    }
    double suggested_latency =
        Pa_GetDeviceInfo(params.device)->defaultLowOutputLatency;
    // printf("SUGGESTED LATENCY: %f\n", suggested_latency);
    return suggested_latency;
}

void pa_teardown(void)
{
    //  time to go home!
    PaError err;
    err = Pa_Terminate();
    if (err != paNoError)
    {
        printf("Errrrr while terminating Portaudio: %s\n",
               Pa_GetErrorText(err));
        exit(-1);
    }
    exit(0);
}

void get_chord_compat_keys(int keynum, int vals[4])
{
    vals[0] = keynum;
    for (int i = 1; i < 4; i++)
    {
        int randy = rand() % 6;
        while (is_int_member_in_array(randy, vals, 4))
        {
            randy = rand() % 6;
        }
        vals[i] = randy;
    }
}

int get_chord_type(unsigned int scale_degree)
{
    if (scale_degree == 0 || scale_degree == 3 || scale_degree == 4 ||
        scale_degree == 7)
        return MAJOR_CHORD;
    else if (scale_degree == 1 || scale_degree == 2 || scale_degree == 5)
        return MINOR_CHORD;
    else if (scale_degree == 6)
        return DIMINISHED_CHORD;
    else
        return -1;
}

void get_midi_notes_from_chord(unsigned int note, unsigned int chord_type,
                               int octave, chord_midi_notes *chnotes)
{
    int root_midi = get_midi_note_from_mixer_key(note, octave);

    int third_note = 0;
    int third_midi = 0;
    if (chord_type == MAJOR_CHORD)
        third_note = (note + 4);
    else
        third_note = (note + 3);
    if (third_note >= NUM_KEYS)
        third_midi =
            get_midi_note_from_mixer_key(third_note % NUM_KEYS, octave + 1);
    else
        third_midi = get_midi_note_from_mixer_key(third_note, octave);

    int fifth_note = 0;
    int fifth_midi = 0;
    if (chord_type == DIMINISHED_CHORD)
        fifth_note = note + 6;
    else
        fifth_note = note + 7;
    if (fifth_note >= NUM_KEYS)
        fifth_midi =
            get_midi_note_from_mixer_key(fifth_note % NUM_KEYS, octave + 1);
    else
        fifth_midi = get_midi_note_from_mixer_key(fifth_note, octave);

    //printf("ROOT MIDI:%d THIRD:%d FIFTH:%d\n", root_midi, third_midi,
    //       fifth_midi);

    chnotes->root = root_midi;
    chnotes->third = third_midi;
    chnotes->fifth = fifth_midi;
}
