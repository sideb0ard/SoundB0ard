#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "midi_freq_table.h"
#include "modmatrix.h"

modmatrix *new_modmatrix(void)
{
    modmatrix *m;
    m = (modmatrix *)calloc(1, sizeof(modmatrix));
    if (m == NULL)
        return NULL;

    create_matrix_core(m);

    return m;
}

int get_matrix_size(modmatrix *self)
{
    int sz = 0;
    for (int i = 0; i < MAX_SOURCES * MAX_DESTINATIONS; i++) {
        matrixrow *mr = self->m_matrix_core[i];
        if (mr)
            sz++;
    }
    return sz;
}

void matrix_clear_sources(modmatrix *self)
{
    for (int i = 0; i < MAX_SOURCES; i++)
        self->m_sources[i] = 0.0;
}

void matrix_clear_destinations(modmatrix *self)
{
    memset(self->m_destinations, 0, sizeof(double) * MAX_DESTINATIONS);
    // for (int i = 0; i < MAX_DESTINATIONS; i++)
    //    self->m_destinations[i] = 0.0;
}

void create_matrix_core(modmatrix *self)
{
    if (self->m_matrix_core) {
        delete_matrix_core(self);
    }

    self->m_matrix_core = (matrixrow **)calloc(MAX_SOURCES * MAX_DESTINATIONS,
                                               sizeof(matrixrow *));
}

void clear_matrix_core(modmatrix *self)
{
    if (!self->m_matrix_core)
        return;
    for (int i = 0; i < self->m_num_rows_in_matrix_core; i++) {
        free(self->m_matrix_core[i]);
    }
    self->m_num_rows_in_matrix_core = 0;
}

void delete_matrix_core(modmatrix *self)
{
    printf("DLETE MATRIX CORE CALLED!\n");
    clear_matrix_core(self);
    free(self->m_matrix_core);
}

matrixrow **get_matrix_core(modmatrix *self) { return self->m_matrix_core; }

void set_matrix_core(modmatrix *self, matrixrow **matrix)
{
    if (self->m_matrix_core) {
        clear_matrix_core(self);
        free(self->m_matrix_core);
    }
    self->m_matrix_core = matrix;
    self->m_num_rows_in_matrix_core = get_matrix_size(self);
}

void add_matrix_row(modmatrix *self, matrixrow *row)
{
    if (!self->m_matrix_core)
        create_matrix_core(self);

    if (!matrix_row_exists(self, row->m_source_index, row->m_destination_index))
        self->m_matrix_core[self->m_num_rows_in_matrix_core++] = row;
    else
        free(row);
}

bool matrix_row_exists(modmatrix *self, unsigned sourceidx, unsigned destidx)
{
    if (!self->m_matrix_core)
        return false;

    for (int i = 0; i < self->m_num_rows_in_matrix_core; i++) {
        matrixrow *mr = self->m_matrix_core[i];
        if (mr->m_source_index == sourceidx &&
            mr->m_destination_index == destidx)
            return true;
    }
    return false;
}

bool enable_matrix_row(modmatrix *self, unsigned sourceidx, unsigned destidx,
                       bool enable)
{
    if (!self->m_matrix_core)
        return false;

    for (int i = 0; i < self->m_num_rows_in_matrix_core; i++) {
        matrixrow *mr = self->m_matrix_core[i];
        if (mr->m_source_index == sourceidx &&
            mr->m_destination_index == destidx) {
            mr->m_enable = enable;
            return true;
        }
    }
    return false;
}

bool check_destination_layer(unsigned layer, matrixrow *row)
{
    bool layer0 = false;
    if (row->m_destination_index >= DEST_LFO1_FO &&
        row->m_destination_index <= DEST_ALL_EG_SUSTAIN_OVERRIDE)
        layer0 = true;

    if (layer == 0)
        return layer0;
    if (layer == 1)
        return !layer0;

    return false;
}

matrixrow *create_matrix_row(unsigned src, unsigned dest, double *intensity,
                             double *range, unsigned transformation,
                             bool enable)
{
    matrixrow *row = (matrixrow *)calloc(1, sizeof(matrixrow));
    row->m_source_index = src;
    row->m_destination_index = dest;
    row->m_mod_intensity = intensity;
    row->m_mod_range = range;
    row->m_source_transform = transformation;
    row->m_enable = enable;

    return row;
}

// this is the REAL mod matrix, yehhhhhh!
void do_modulation_matrix(modmatrix *self, unsigned layer)
{

    if (!self->m_matrix_core) {
        printf("NAE MATRIX CORE, MATE!\n");
        return;
    }

    matrix_clear_destinations(self);

    for (int i = 0; i < self->m_num_rows_in_matrix_core; i++) {

        matrixrow *mr = self->m_matrix_core[i];

        if (!mr)
            continue; // shouldn't happen. but jist in case!
        if (!mr->m_enable)
            continue;
        if (!check_destination_layer(layer, mr)) {
            continue;
        }

        double src = self->m_sources[mr->m_source_index];

        switch (mr->m_source_transform) {
        case TRANSFORM_UNIPOLAR_TO_BIPOLAR:
            // src = unipolar_to_bipolar(src);
            break;
        case TRANSFORM_BIPOLAR_TO_UNIPOLAR:
            // src = bipolar_to_unipolar(src);
            break;
        case TRANSFORM_MIDI_TO_ATTENUATION:
            // src = mma_midi_to_atten(src);
            break;
        case TRANSFORM_MIDI_TO_PAN:
            // src = midi_to_pan_value(src);
            break;
        case TRANSFORM_MIDI_SWITCH:
            src = src > 63 ? 1.0 : 0.0;
            break;
        case TRANSFORM_MIDI_TO_BIPOLAR:
            // src = midi_to_bipolar(src);
            break;
        case TRANSFORM_NOTE_NUMBER_TO_FREQUENCY:
            src = get_midi_freq(src);
            break;
        case TRANSFORM_MIDI_NORMALIZE:
            src /= 127.0;
            break;
        case TRANSFORM_INVERT_MIDI_NORMALIZE:
            src /= 127.0;
            src = 1.0 - src;
            break;
        default:
            break;
        }

        // destination += source*intensity*range
        double modval = src * (*mr->m_mod_intensity) * (*mr->m_mod_range);

        switch (mr->m_destination_index) {
        case DEST_ALL_OSC_FO:
            self->m_destinations[DEST_OSC1_FO] += modval;
            self->m_destinations[DEST_OSC2_FO] += modval;
            self->m_destinations[DEST_OSC2_FO] += modval;
            self->m_destinations[DEST_OSC4_FO] += modval;
            self->m_destinations[DEST_ALL_OSC_FO] += modval;
        case DEST_ALL_OSC_PULSEWIDTH:
            self->m_destinations[DEST_OSC1_PULSEWIDTH] += modval;
            self->m_destinations[DEST_OSC2_PULSEWIDTH] += modval;
            self->m_destinations[DEST_OSC2_PULSEWIDTH] += modval;
            self->m_destinations[DEST_OSC4_PULSEWIDTH] += modval;
            self->m_destinations[DEST_ALL_OSC_PULSEWIDTH] += modval;
        case DEST_ALL_OSC_FO_RATIO:
            self->m_destinations[DEST_OSC1_FO_RATIO] += modval;
            self->m_destinations[DEST_OSC2_FO_RATIO] += modval;
            self->m_destinations[DEST_OSC2_FO_RATIO] += modval;
            self->m_destinations[DEST_OSC4_FO_RATIO] += modval;
            self->m_destinations[DEST_ALL_OSC_FO_RATIO] += modval;
        case DEST_ALL_OSC_OUTPUT_AMP:
            self->m_destinations[DEST_OSC1_OUTPUT_AMP] += modval;
            self->m_destinations[DEST_OSC2_OUTPUT_AMP] += modval;
            self->m_destinations[DEST_OSC2_OUTPUT_AMP] += modval;
            self->m_destinations[DEST_OSC4_OUTPUT_AMP] += modval;
            self->m_destinations[DEST_ALL_OSC_OUTPUT_AMP] += modval;
        case DEST_ALL_LFO_FO:
            self->m_destinations[DEST_LFO1_FO] += modval;
            self->m_destinations[DEST_LFO2_FO] += modval;
            self->m_destinations[DEST_ALL_LFO_FO] += modval;
        case DEST_ALL_LFO_OUTPUT_AMP:
            self->m_destinations[DEST_LFO1_OUTPUT_AMP] += modval;
            self->m_destinations[DEST_LFO2_OUTPUT_AMP] += modval;
            self->m_destinations[DEST_ALL_LFO_OUTPUT_AMP] += modval;
            break;
        case DEST_ALL_FILTER_FC:
            // printf("Writing to DEST_FILTER1_FC! %f\n", modval);
            self->m_destinations[DEST_FILTER1_FC] += modval;
            self->m_destinations[DEST_FILTER2_FC] += modval;
            self->m_destinations[DEST_ALL_FILTER_FC] += modval;
            break;

        case DEST_ALL_FILTER_KEYTRACK:
            self->m_destinations[DEST_FILTER1_KEYTRACK] += modval;
            self->m_destinations[DEST_FILTER2_KEYTRACK] += modval;
            self->m_destinations[DEST_ALL_FILTER_KEYTRACK] += modval;
            break;

        case DEST_ALL_EG_ATTACK_SCALING:
            self->m_destinations[DEST_EG1_ATTACK_SCALING] += modval;
            self->m_destinations[DEST_EG2_ATTACK_SCALING] += modval;
            self->m_destinations[DEST_EG3_ATTACK_SCALING] += modval;
            self->m_destinations[DEST_EG4_ATTACK_SCALING] += modval;
            self->m_destinations[DEST_ALL_EG_ATTACK_SCALING] += modval;
            break;

        case DEST_ALL_EG_DECAY_SCALING:
            self->m_destinations[DEST_EG1_DECAY_SCALING] += modval;
            self->m_destinations[DEST_EG2_DECAY_SCALING] += modval;
            self->m_destinations[DEST_EG3_DECAY_SCALING] += modval;
            self->m_destinations[DEST_EG4_DECAY_SCALING] += modval;
            self->m_destinations[DEST_ALL_EG_DECAY_SCALING] += modval;
            break;

        case DEST_ALL_EG_SUSTAIN_OVERRIDE:
            self->m_destinations[DEST_EG1_SUSTAIN_OVERRIDE] += modval;
            self->m_destinations[DEST_EG2_SUSTAIN_OVERRIDE] += modval;
            self->m_destinations[DEST_EG3_SUSTAIN_OVERRIDE] += modval;
            self->m_destinations[DEST_EG4_SUSTAIN_OVERRIDE] += modval;
            break;
        default:
            self->m_destinations[mr->m_destination_index] += modval;
        }
    }
}
