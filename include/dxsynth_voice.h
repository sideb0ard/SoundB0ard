#pragma once

#include "qblimited_oscillator.h"
#include "voice.h"
#include "wt_oscillator.h"

enum { DX_LFO_DEST_NONE, DX_LFO_DEST_AMP_MOD, DX_LFO_DEST_VIBRATO };

struct DXSynthVoice : public Voice {
  DXSynthVoice();
  ~DXSynthVoice() = default;

  QBLimitedOscillator m_op1;
  QBLimitedOscillator m_op2;
  QBLimitedOscillator m_op3;
  QBLimitedOscillator m_op4;

  // WtOscillator m_op1;
  // WtOscillator m_op2;
  // WtOscillator m_op3;
  // WtOscillator m_op4;

  double m_op1_feedback;
  double m_op2_feedback;
  double m_op3_feedback;
  double m_op4_feedback;

  unsigned int m_lfo_mod_dest;  // none, ampmod, or vibrato

  void InitializeModMatrix(ModulationMatrix *matrix) override;
  void InitGlobalParameters(GlobalSynthParams *sp) override;
  bool CanNoteOff() override;
  bool IsVoiceDone() override;
  void PrepareForPlay() override;
  void Update() override;
  void Reset() override;
  bool DoVoice(double *left_output, double *right_output) override;

  void SetLFO1Destination(unsigned int op, unsigned int dest);
  void SetOutputEGs();
  void SetSustainOverride(unsigned int op, bool b);
};
