#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

#include "midi_freq_table.h"
#include "modmatrix.h"
#include "synthfunctions.h"
#include "utils.h"

const char *s_source_enum_to_name[] = {"SOURCE_NONE",
                                       "SOURCE_LFO1",
                                       "SOURCE_LFO2",
                                       "SOURCE_LFO1Q",
                                       "SOURCE_LFO2Q",
                                       "SOURCE_EG1",
                                       "SOURCE_EG2",
                                       "SOURCE_EG3",
                                       "SOURCE_EG4",
                                       "SOURCE_BIASED_EG1",
                                       "SOURCE_BIASED_EG2",
                                       "SOURCE_BIASED_EG3",
                                       "SOURCE_BIASED_EG4",
                                       "SOURCE_VELOCITY",
                                       "SOURCE_MIDI_VOLUME_CC07",
                                       "SOURCE_MIDI_PAN_CC10",
                                       "SOURCE_MIDI_EXPRESSION_CC11",
                                       "SOURCE_MODWHEEL",
                                       "SOURCE_PITCHBEND",
                                       "SOURCE_SUSTAIN_PEDAL",
                                       "SOURCE_MIDI_NOTE_NUM",
                                       "SOURCE_MIDI_JS_X",
                                       "SOURCE_MIDI_JS_Y",
                                       "MAX_SOURCES"};

const char *s_dest_enum_to_name[] = {
    "DEST_NONE",
    // --- LAYER 0 DESTINATIONS
    "DEST_LFO1_FO", "DEST_LFO2_FO", "DEST_ALL_LFO_FO", "DEST_LFO1_OUTPUT_AMP",
    "DEST_LFO2_OUTPUT_AMP", "DEST_ALL_LFO_OUTPUT_AMP",
    "DEST_EG1_ATTACK_SCALING", "DEST_EG2_ATTACK_SCALING",
    "DEST_EG3_ATTACK_SCALING", "DEST_EG4_ATTACK_SCALING",
    "DEST_ALL_EG_ATTACK_SCALING", "DEST_EG1_DECAY_SCALING",
    "DEST_EG2_DECAY_SCALING", "DEST_EG3_DECAY_SCALING",
    "DEST_EG4_DECAY_SCALING", "DEST_ALL_EG_DECAY_SCALING",
    "DEST_EG1_SUSTAIN_OVERRIDE", "DEST_EG2_SUSTAIN_OVERRIDE",
    "DEST_EG3_SUSTAIN_OVERRIDE", "DEST_EG4_SUSTAIN_OVERRIDE",
    "DEST_ALL_EG_SUSTAIN_OVERRIDE", // <- keep this last
    // --- END OF LAYER 0 DESTINATIONS

    // --- LAYER 1 DESTINATIONS
    "DEST_HARD_SYNC_RATIO", "DEST_OSC1_FO", "DEST_OSC2_FO", "DEST_OSC3_FO",
    "DEST_OSC4_FO", "DEST_ALL_OSC_FO", "DEST_OSC1_PULSEWIDTH",
    "DEST_OSC2_PULSEWIDTH", "DEST_OSC3_PULSEWIDTH", "DEST_OSC4_PULSEWIDTH",
    "DEST_ALL_OSC_PULSEWIDTH", "DEST_OSC1_FO_RATIO", "DEST_OSC2_FO_RATIO",
    "DEST_OSC3_FO_RATIO", "DEST_OSC4_FO_RATIO", "DEST_ALL_OSC_FO_RATIO",
    "DEST_OSC1_OUTPUT_AMP", "DEST_OSC2_OUTPUT_AMP", "DEST_OSC3_OUTPUT_AMP",
    "DEST_OSC4_OUTPUT_AMP", "DEST_ALL_OSC_OUTPUT_AMP", "DEST_FILTER1_FC",
    "DEST_FILTER2_FC", "DEST_ALL_FILTER_FC", "DEST_FILTER1_KEYTRACK",
    "DEST_FILTER2_KEYTRACK",
    "DEST_ALL_FILTER_KEYTRACK", // the control value, overriding the GUI
    "DEST_FILTER1_Q", "DEST_FILTER2_Q", "DEST_ALL_FILTER_Q", "DEST_VS_AC_AXIS",
    "DEST_VS_BD_AXIS", "DEST_DCA_VELOCITY", "DEST_DCA_EG", "DEST_DCA_AMP",
    "DEST_DCA_PAN",
    // --- END OF LAYER 1 DESTINATIONS
    "MAX_DESTINATIONS"};

std::shared_ptr<ModMatrixRow> CreateMatrixRow(unsigned src, unsigned dest,
                                              double *intensity, double *range,
                                              unsigned transformation,
                                              bool enable)
{
    auto row = std::make_shared<ModMatrixRow>();
    row->source_index = src;
    row->destination_index = dest;
    row->mod_intensity = intensity;
    row->mod_range = range;
    row->source_transform = transformation;
    row->enable = enable;

    return row;
}

int ModulationMatrix::GetMatrixSize() { return matrix_core.size(); }

void ModulationMatrix::ClearSources()
{
    std::fill(std::begin(sources), std::end(sources), 0);
}

inline void ModulationMatrix::ClearDestinations()
{
    std::fill(std::begin(destinations), std::end(destinations), 0);
}

void ModulationMatrix::AddMatrixRow(std::shared_ptr<ModMatrixRow> row)
{

    if (!MatrixRowExists(row->source_index, row->destination_index))
        matrix_core.push_back(row);
}

bool ModulationMatrix::MatrixRowExists(unsigned sourceidx, unsigned destidx)
{
    for (auto mr : matrix_core)
    {
        if (mr->source_index == sourceidx && mr->destination_index == destidx)
            return true;
    }
    return false;
}

bool ModulationMatrix::EnableMatrixRow(unsigned sourceidx, unsigned destidx,
                                       bool enable)
{
    for (auto mr : matrix_core)
    {
        if (mr->source_index == sourceidx && mr->destination_index == destidx)
        {
            mr->enable = enable;
            return true;
        }
    }
    return false;
}

bool ModulationMatrix::CheckDestinationLayer(unsigned layer,
                                             std::shared_ptr<ModMatrixRow> row)
{
    bool layer0 = false;
    if (row->destination_index >= DEST_LFO1_FO &&
        row->destination_index <= DEST_ALL_EG_SUSTAIN_OVERRIDE)
        layer0 = true;

    if (layer == 0)
        return layer0;
    if (layer == 1)
        return !layer0;

    return false;
}

// this is the REAL mod matrix, yehhhhhh!
void ModulationMatrix::DoModMatrix(unsigned layer)
{
    ClearDestinations();

    for (auto mr : matrix_core)
    {

        if (!mr->enable)
            continue;

        if (!CheckDestinationLayer(layer, mr))
            continue;

        double src = sources[mr->source_index];
        // std::cout << "SRC VAL:" << src << std::endl;

        switch (mr->source_transform)
        {
        case TRANSFORM_UNIPOLAR_TO_BIPOLAR:
            src = unipolar_to_bipolar(src);
            break;
        case TRANSFORM_BIPOLAR_TO_UNIPOLAR:
            src = bipolar_to_unipolar(src);
            break;
        case TRANSFORM_MIDI_TO_ATTENUATION:
            src = mma_midi_to_atten_db(src);
            break;
        case TRANSFORM_MIDI_TO_PAN:
            src = midi_to_pan_value(src);
            break;
        case TRANSFORM_MIDI_SWITCH:
            src = src > 63 ? 1.0 : 0.0;
            break;
        case TRANSFORM_MIDI_TO_BIPOLAR:
            src = midi_to_bipolar(src);
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
        double modval = src * (*mr->mod_intensity) * (*mr->mod_range);

        // if (mr->source_index == 5)
        //{
        //    std::cout << "DO MOD MATRIX: src:" << mr->source_index
        //              << " Dest:" << mr->destination_index << " val:" <<
        //              modval
        //              << std ::endl;
        //}
        // if (mr->destination_index == DEST_DCA_EG)
        //{
        //    std::cout << "DEST OTPUT  : src:" << mr->source_index
        //              << " DEST:" << mr->destination_index << " val:" <<
        //              modval
        //              << std ::endl;
        //}
        switch (mr->destination_index)
        {
        case DEST_ALL_OSC_FO:
            destinations[DEST_OSC1_FO] += modval;
            destinations[DEST_OSC2_FO] += modval;
            destinations[DEST_OSC2_FO] += modval;
            destinations[DEST_OSC4_FO] += modval;
            destinations[DEST_ALL_OSC_FO] += modval;
        case DEST_ALL_OSC_PULSEWIDTH:
            destinations[DEST_OSC1_PULSEWIDTH] += modval;
            destinations[DEST_OSC2_PULSEWIDTH] += modval;
            destinations[DEST_OSC2_PULSEWIDTH] += modval;
            destinations[DEST_OSC4_PULSEWIDTH] += modval;
            destinations[DEST_ALL_OSC_PULSEWIDTH] += modval;
        case DEST_ALL_OSC_FO_RATIO:
            destinations[DEST_OSC1_FO_RATIO] += modval;
            destinations[DEST_OSC2_FO_RATIO] += modval;
            destinations[DEST_OSC2_FO_RATIO] += modval;
            destinations[DEST_OSC4_FO_RATIO] += modval;
            destinations[DEST_ALL_OSC_FO_RATIO] += modval;
        case DEST_ALL_OSC_OUTPUT_AMP:
            destinations[DEST_OSC1_OUTPUT_AMP] += modval;
            destinations[DEST_OSC2_OUTPUT_AMP] += modval;
            destinations[DEST_OSC2_OUTPUT_AMP] += modval;
            destinations[DEST_OSC4_OUTPUT_AMP] += modval;
            destinations[DEST_ALL_OSC_OUTPUT_AMP] += modval;
        case DEST_ALL_LFO_FO:
            destinations[DEST_LFO1_FO] += modval;
            destinations[DEST_LFO2_FO] += modval;
            destinations[DEST_ALL_LFO_FO] += modval;
        case DEST_ALL_LFO_OUTPUT_AMP:
            destinations[DEST_LFO1_OUTPUT_AMP] += modval;
            destinations[DEST_LFO2_OUTPUT_AMP] += modval;
            destinations[DEST_ALL_LFO_OUTPUT_AMP] += modval;
            break;
        case DEST_ALL_FILTER_FC:
            // printf("Writing to DEST_FILTER1_FC! %f\n", modval);
            destinations[DEST_FILTER1_FC] += modval;
            destinations[DEST_FILTER2_FC] += modval;
            destinations[DEST_ALL_FILTER_FC] += modval;
            break;

        case DEST_ALL_FILTER_KEYTRACK:
            destinations[DEST_FILTER1_KEYTRACK] += modval;
            destinations[DEST_FILTER2_KEYTRACK] += modval;
            destinations[DEST_ALL_FILTER_KEYTRACK] += modval;
            break;

        case DEST_ALL_EG_ATTACK_SCALING:
            destinations[DEST_EG1_ATTACK_SCALING] += modval;
            destinations[DEST_EG2_ATTACK_SCALING] += modval;
            destinations[DEST_EG3_ATTACK_SCALING] += modval;
            destinations[DEST_EG4_ATTACK_SCALING] += modval;
            destinations[DEST_ALL_EG_ATTACK_SCALING] += modval;
            break;

        case DEST_ALL_EG_DECAY_SCALING:
            destinations[DEST_EG1_DECAY_SCALING] += modval;
            destinations[DEST_EG2_DECAY_SCALING] += modval;
            destinations[DEST_EG3_DECAY_SCALING] += modval;
            destinations[DEST_EG4_DECAY_SCALING] += modval;
            destinations[DEST_ALL_EG_DECAY_SCALING] += modval;
            break;

        case DEST_ALL_EG_SUSTAIN_OVERRIDE:
            destinations[DEST_EG1_SUSTAIN_OVERRIDE] += modval;
            destinations[DEST_EG2_SUSTAIN_OVERRIDE] += modval;
            destinations[DEST_EG3_SUSTAIN_OVERRIDE] += modval;
            destinations[DEST_EG4_SUSTAIN_OVERRIDE] += modval;
            break;
        default:
            destinations[mr->destination_index] += modval;
        }
    }
}

std::vector<std::shared_ptr<ModMatrixRow>> &ModulationMatrix::GetModMatrixCore()
{
    return matrix_core;
}

void ModulationMatrix::SetModMatrixCore(
    std::vector<std::shared_ptr<ModMatrixRow>> &core)
{
    matrix_core = core;
}
