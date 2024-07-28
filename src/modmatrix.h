#pragma once

#include <stdbool.h>

#include <array>
#include <memory>
#include <vector>

enum MatrixSources {
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
};

enum MatrixDestinations {
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
  DEST_ALL_EG_SUSTAIN_OVERRIDE,  // <- keep this last
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
  DEST_ALL_FILTER_KEYTRACK,  // the control value, overriding the GUI
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
};

enum MatrixTransformations {
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
};

struct ModMatrixRow {
  unsigned source_index{0};
  unsigned destination_index{0};
  double *mod_intensity{nullptr};
  double *mod_range{nullptr};
  unsigned source_transform{0};
  bool enable{false};
};

struct ModulationMatrix {
  ModulationMatrix() = default;
  ~ModulationMatrix() = default;

  std::vector<std::shared_ptr<ModMatrixRow>> &GetModMatrixCore();
  void SetModMatrixCore(std::vector<std::shared_ptr<ModMatrixRow>> &matrix);

  int GetMatrixSize();

  void ClearMatrix();
  void ClearSources();
  void ClearDestinations();

  void AddMatrixRow(std::shared_ptr<ModMatrixRow> row);
  bool MatrixRowExists(unsigned sourceidx, unsigned destidx);

  bool EnableMatrixRow(unsigned sourceidx, unsigned destidx, bool enable);

  bool CheckDestinationLayer(unsigned layer, std::shared_ptr<ModMatrixRow> row);

  void DoModMatrix(unsigned layer);

  std::vector<std::shared_ptr<ModMatrixRow>> matrix_core;
  std::array<double, MAX_SOURCES> sources{};
  std::array<double, MAX_DESTINATIONS> destinations{};
};

std::shared_ptr<ModMatrixRow> CreateMatrixRow(unsigned src, unsigned dest,
                                              double *intensity, double *range,
                                              unsigned transformation,
                                              bool enable);
