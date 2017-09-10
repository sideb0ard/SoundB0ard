#pragma once

void pa_setup(void);
void pa_teardown(void);

typedef struct audio_buffer_details {
    char filename[1024];
    int num_channels;
    int sample_rate;
    int buffer_length;
} audio_buffer_details;

typedef struct chord_midi_notes {
    int root;
    int third;
    int fifth;
} chord_midi_notes;

chord_midi_notes get_midi_notes_from_char_chord(const char *chord);
