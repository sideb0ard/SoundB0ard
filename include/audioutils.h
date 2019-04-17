#pragma once

double pa_setup(void);
void pa_teardown(void);

typedef struct audio_buffer_details
{
    char filename[1024];
    int num_channels;
    int sample_rate;
    int buffer_length;
} audio_buffer_details;

typedef struct chord_midi_notes
{
    int root;
    int third;
    int fifth;
    int seventh;
} chord_midi_notes;

enum
{
    MAJOR_CHORD,
    MINOR_CHORD,
    DIMINISHED_CHORD,
    NUM_CHORD_TYPES,
}; // chord type

int get_chord_type(unsigned int scale_degree);
void get_midi_notes_from_chord(unsigned int note, unsigned int chord_type,
                               int octave, chord_midi_notes *chnotes);
bool is_midi_note_in_key(unsigned int midi_note, unsigned int key);
