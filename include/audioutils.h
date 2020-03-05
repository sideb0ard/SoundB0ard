#pragma once

#include <defjams.h>

#include <string>

double pa_setup(void);
void pa_teardown(void);

typedef struct audio_buffer_details
{
    char filename[1024];
    int num_channels;
    int sample_rate;
    int buffer_length;
} audio_buffer_details;

int get_chord_type(unsigned int scale_degree);
void get_midi_notes_from_chord(unsigned int note, unsigned int chord_type,
                               int octave, chord_midi_notes *chnotes);

int GetThird(int midi_note, char key);
int GetFifth(int midi_note, char key);
int GetNthDegree(int midi_note, int degree, char key);
int Scale(int midi_note, char key);
bool IsMidiNoteInKey(int midi_note, char key);
bool IsNote(std::string input);
