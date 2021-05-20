#pragma once

#include <stdbool.h>
#include <wchar.h>

typedef enum
{
    SOURCE_NONE,
    SOURCE_LFO1,
    SOURCE_LFO2,
    SOURCE_LFO1Q,
    SOURCE_LFO2Q,
    SOURCE_EG1,
    SOURCE_EG2,
    SOURCE_EG3,
    SOURCE_EG4,
    SOURCE_BIASED_EG1,
    SOURCE_BIASED_EG2,
    SOURCE_BIASED_EG3,
    SOURCE_BIASED_EG4,
    SOURCE_VELOCITY,
    SOURCE_MIDI_VOLUME_CC07,
    SOURCE_MIDI_PAN_CC10,
    SOURCE_MIDI_EXPRESSION_CC11,
    SOURCE_MODWHEEL,
    SOURCE_PITCHBEND,
    SOURCE_SUSTAIN_PEDAL,
    SOURCE_MIDI_NOTE_NUM,
    SOURCE_MIDI_JS_X,
    SOURCE_MIDI_JS_Y,
    MAX_SOURCES
} matrix_sources;

typedef enum
{
    DEST_NONE,

    // --- LAYER 0 DESTINATIONS
    DEST_LFO1_FO,
    DEST_LFO2_FO,
    DEST_ALL_LFO_FO,
    DEST_LFO1_OUTPUT_AMP,
    DEST_LFO2_OUTPUT_AMP,
    DEST_ALL_LFO_OUTPUT_AMP,
    DEST_EG1_ATTACK_SCALING,
    DEST_EG2_ATTACK_SCALING,
    DEST_EG3_ATTACK_SCALING,
    DEST_EG4_ATTACK_SCALING,
    DEST_ALL_EG_ATTACK_SCALING,
    DEST_EG1_DECAY_SCALING,
    DEST_EG2_DECAY_SCALING,
    DEST_EG3_DECAY_SCALING,
    DEST_EG4_DECAY_SCALING,
    DEST_ALL_EG_DECAY_SCALING,
    DEST_EG1_SUSTAIN_OVERRIDE,
    DEST_EG2_SUSTAIN_OVERRIDE,
    DEST_EG3_SUSTAIN_OVERRIDE,
    DEST_EG4_SUSTAIN_OVERRIDE,
    DEST_ALL_EG_SUSTAIN_OVERRIDE, // <- keep this last
    // --- END OF LAYER 0 DESTINATIONS

    // --- LAYER 1 DESTINATIONS
    DEST_HARD_SYNC_RATIO,
    DEST_OSC1_FO,
    DEST_OSC2_FO,
    DEST_OSC3_FO,
    DEST_OSC4_FO,
    DEST_ALL_OSC_FO,
    DEST_OSC1_PULSEWIDTH,
    DEST_OSC2_PULSEWIDTH,
    DEST_OSC3_PULSEWIDTH,
    DEST_OSC4_PULSEWIDTH,
    DEST_ALL_OSC_PULSEWIDTH,
    DEST_OSC1_FO_RATIO,
    DEST_OSC2_FO_RATIO,
    DEST_OSC3_FO_RATIO,
    DEST_OSC4_FO_RATIO,
    DEST_ALL_OSC_FO_RATIO,
    DEST_OSC1_OUTPUT_AMP,
    DEST_OSC2_OUTPUT_AMP,
    DEST_OSC3_OUTPUT_AMP,
    DEST_OSC4_OUTPUT_AMP,
    DEST_ALL_OSC_OUTPUT_AMP,
    DEST_FILTER1_FC,
    DEST_FILTER2_FC,
    DEST_ALL_FILTER_FC,
    DEST_FILTER1_KEYTRACK,
    DEST_FILTER2_KEYTRACK,
    DEST_ALL_FILTER_KEYTRACK, // the control value, overriding the GUI
    DEST_FILTER1_Q,
    DEST_FILTER2_Q,
    DEST_ALL_FILTER_Q,
    DEST_VS_AC_AXIS,
    DEST_VS_BD_AXIS,
    DEST_DCA_VELOCITY,
    DEST_DCA_EG,
    DEST_DCA_AMP,
    DEST_DCA_PAN,
    // --- END OF LAYER 1 DESTINATIONS

    MAX_DESTINATIONS
} matrix_destinations;

typedef enum
{
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

typedef struct matrixrow
{
    unsigned m_source_index;
    unsigned m_destination_index;
    double *m_mod_intensity{nullptr};
    double *m_mod_range{nullptr};
    unsigned m_source_transform;
    bool m_enable;
} matrixrow;

typedef struct modmatrix
{
    matrixrow **m_matrix_core;
    int m_num_rows_in_matrix_core;
    double m_sources[MAX_SOURCES];
    double m_destinations[MAX_DESTINATIONS];
} modmatrix;

modmatrix *new_modmatrix(void);

int get_matrix_size(modmatrix *self);

void matrix_clear_sources(modmatrix *self);
void matrix_clear_destinations(modmatrix *self);

void create_matrix_core(modmatrix *self);
void clear_matrix_core(modmatrix *self);
void delete_matrix_core(modmatrix *self);

matrixrow **get_matrix_core(modmatrix *self);
void set_matrix_core(modmatrix *self, matrixrow **matrix);

void add_matrix_row(modmatrix *self, matrixrow *row);
bool matrix_row_exists(modmatrix *self, unsigned sourceidx, unsigned destidx);
bool enable_matrix_row(modmatrix *self, unsigned sourceidx, unsigned destidx,
                       bool enable);

bool check_destination_layer(unsigned layer, matrixrow *row);
void do_modulation_matrix(modmatrix *self, unsigned layer);

void print_modulation_matrix(modmatrix *self);
void print_modulation_matrix_info_lfo1(modmatrix *self, wchar_t *status_string);
void print_modulation_matrix_info_eg1(modmatrix *self, wchar_t *status_string);

matrixrow *create_matrix_row(unsigned src, unsigned dest, double *intensity,
                             double *range, unsigned transformation,
                             bool enable);
