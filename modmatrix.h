#pragma once

#include <stdbool.h>


typedef enum {
    SOURCE_NONE,
    SOURCE_LFO1,
    SOURCE_LFO2,
    SOURCE_EG1,
    SOURCE_EG1_BIASED,
    SOURCE_MIDI_NOTE_NUM,
    MAX_SOURCES
} matrix_sources;


typedef enum {
    DEST_NONE,

    // layer 0 Destinations
    DEST_LFO1_FQ,
    DEST_LFO2_FQ,
    DEST_ALL_LFO_FQ,

    DEST_ALL_EG_SUSTAIN_OVERRIDE,
    // layer 1 destinations
    DEST_OSC1_FQ,
    DEST_OSC2_FQ,
    DEST_FILTER1_FC,
    DEST_DCA_EG_IN,
    
    // universal dests
    DEST_ALL_OSC_FQ,
    MAX_DESTINATIONS
} matrix_destinations;


typedef enum {
   TRANSFORM_NONE,
   TRANSFORM_UNIPOLAR_TO_BIPOLAR,
   TRANSFORM_BIPOLAR_TO_UNIPOLAR,
   TRANSFORM_MIDI_NORMALIZE,
   TRANSFORM_INVERT_MIDI_NORMALIZE,
   TRANSFORM_MIDI_TO_BIPOLAR,
   TRANSFORM_MIDI_TO_PAN,
   TRANSFORM_MIDI_SWITCH,
   TRANSFORM_MIDI_TO_ATTENUATION,
   TRANSFORM_NOTE_NUMBER_TO_FREQUENCY,
   MAX_TRANSFORMS /* not needed? */
} matrix_transformations;


typedef struct {
    unsigned m_source_index;
    unsigned m_destination_index;
    double *m_mod_intensity;
    double *m_mod_range;
    unsigned m_source_transform;
    bool m_enable;
} matrixrow;


typedef struct {
    matrixrow **m_matrix_core;
    int m_num_rows_in_matrix_core;
    double m_sources[MAX_SOURCES];
    double m_destinations[MAX_DESTINATIONS];
} modmatrix;


modmatrix *new_modmatrix(void);

int get_matrix_size(modmatrix *self);

void clear_sources(modmatrix *self);
void clear_destinations(modmatrix *self);

void create_matrix_core(modmatrix *self);
void clear_matrix_core(modmatrix *self);
void delete_matrix_core(modmatrix *self);

matrixrow **get_matrix_core(modmatrix *self);
void set_matrix_core(modmatrix *self, matrixrow **matrix);

void add_matrix_row(modmatrix *self, matrixrow *row);
bool matrix_row_exists(modmatrix *self, unsigned sourceidx, unsigned destidx);
bool enable_matrix_row(modmatrix *self, unsigned sourceidx, unsigned destidx, bool enable);

bool check_destination_layer(unsigned layer, matrixrow *row);
void do_modulation_matrix(modmatrix *self, unsigned layer);


