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

    // printf("ROOT MIDI:%d THIRD:%d FIFTH:%d\n", root_midi, third_midi,
    //       fifth_midi);

    int randy = rand() % 100;
    if (randy > 90)
    {
        int randy_root = 0;
        int randy_fifth = 0;

        if (randy > 97)
            randy_root = root_midi + 12;
        else if (randy > 95)
            randy_root = root_midi - 12;
        else if (randy > 92)
            randy_fifth = fifth_midi + 12;
        else if (randy > 90)
            randy_fifth = fifth_midi - 12;

        if (randy_root > 0 && randy_root < 128)
            root_midi = randy_root;
        if (randy_fifth > 0 && randy_fifth < 128)
            fifth_midi = randy_fifth;
    }

    chnotes->root = root_midi;
    chnotes->third = third_midi;
    chnotes->fifth = fifth_midi;
}

int GetScaleIndex(int note, int key)
{
    if (!IsMidiNoteInKey(note, key))
        return -1;
    note = note % 12;
    key = key % 12;
    if (note == key)
        return 0;
    else if (note == key + 2)
        return 1;
    else if (note == key + 4)
        return 2;
    else if (note == key + 5)
        return 3;
    else if (note == key + 7)
        return 5;
    else if (note == key + 9)
        return 6;
    else if (note == key + 11)
        return 7;
    return -1;
}

int GetStepsToNextDegree(int scale_index)
{
    if (scale_index > 7)
        return -1;
    switch (scale_index)
    {
    case (0):
        return 2;
    case (1):
        return 2;
    case (2):
        return 1;
    case (3):
        return 2;
    case (4):
        return 2;
    case (5):
        return 2;
    case (6):
        return 1;
    case (7):
        return 2;
    }
    return -1;
}

// returns an int to indicate midi num representing n'th
// degree from input note
int GetNthDegree(int note, int degree, int key)
{
    if (!IsMidiNoteInKey(note, key))
        return -1;
    int scale_index = GetScaleIndex(note, key);
    int return_midi_num = note;
    for (int i = 0; i < degree; i++)
    {
        return_midi_num += GetStepsToNextDegree(scale_index);
    }
    return return_midi_num;
}

bool IsMidiNoteInKey(unsigned int note, unsigned int key)
{
    // printf("NOTE is %d and KEY is %d\n", note, key);
    // western scale is 2 2 1 2 2 2 1
    note = note % 12;
    key = key % 12;
    if (note == key)
        return true;
    else if (note == key + 2)
        return true;
    else if (note == key + 4)
        return true;
    else if (note == key + 5)
        return true;
    else if (note == key + 7)
        return true;
    else if (note == key + 9)
        return true;
    else if (note == key + 11)
        return true;
    else if (note == key + 12)
        return true;
    return false;
}

bool IsNote(std::string input)
{
    if (input.size() > 0)
    {
        char firstchar = ::tolower(input[0]);
        if (firstchar == 'a' || firstchar == 'b' || firstchar == 'c' ||
            firstchar == 'd' || firstchar == 'e' || firstchar == 'f' ||
            firstchar == 'g')
            return true;
    }

    return false;
}
