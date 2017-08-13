#pragma once

void pa_setup(void);
void pa_teardown(void);

typedef struct chord_midi_notes {
    int root;
    int third;
    int fifth;
} chord_midi_notes;

chord_midi_notes get_midi_notes_from_char_chord(const char *chord);
