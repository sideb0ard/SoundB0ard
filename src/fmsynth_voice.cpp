#include <fmsynth_voice.h>
#include <stdlib.h>
#include <utils.h>

#include <iostream>

#define IMAX 4.0

FMSynthVoice::FMSynthVoice() {
  // attach oscillators to base class
  m_osc1 = &m_op1;
  m_osc2 = &m_op2;
  m_osc3 = &m_op3;
  m_osc4 = &m_op4;

  m_op1_feedback = 0.0;
  m_op2_feedback = 0.0;
  m_op3_feedback = 0.0;
  m_op4_feedback = 0.0;
}

void FMSynthVoice::InitGlobalParameters(GlobalSynthParams *sp) {
  Voice::InitGlobalParameters(sp);
  m_global_voice_params->op1_feedback = 1.0;
  m_global_voice_params->op2_feedback = 1.0;
  m_global_voice_params->op3_feedback = 1.0;
  m_global_voice_params->op4_feedback = 1.0;
  m_global_voice_params->lfo1_osc_mod_intensity = 1.0;
}

void FMSynthVoice::InitializeModMatrix(ModulationMatrix *matrix) {
  Voice::InitializeModMatrix(matrix);

  std::shared_ptr<ModMatrixRow> row;

  // LFO1 -> DEST_OSC1_OUTPUT_AMP
  row = CreateMatrixRow(SOURCE_LFO1, DEST_OSC1_OUTPUT_AMP,
                        &m_global_voice_params->lfo1_osc_mod_intensity,
                        &m_default_mod_range, TRANSFORM_BIPOLAR_TO_UNIPOLAR,
                        false);
  matrix->AddMatrixRow(row);

  // LFO1 -> DEST_OSC1_FO (VIBRATO)
  row = CreateMatrixRow(
      SOURCE_LFO1, DEST_OSC1_FO, &m_global_voice_params->lfo1_osc_mod_intensity,
      &m_global_voice_params->osc_fo_mod_range, TRANSFORM_NONE, false);
  matrix->AddMatrixRow(row);

  // LFO1 -> DEST_OSC2_OUTPUT_AMP
  row = CreateMatrixRow(SOURCE_LFO1, DEST_OSC2_OUTPUT_AMP,
                        &m_global_voice_params->lfo1_osc_mod_intensity,
                        &m_default_mod_range, TRANSFORM_BIPOLAR_TO_UNIPOLAR,
                        false);
  matrix->AddMatrixRow(row);

  // LFO1 -> DEST_OSC2_FO (VIBRATO)
  row = CreateMatrixRow(
      SOURCE_LFO1, DEST_OSC2_FO, &m_global_voice_params->lfo1_osc_mod_intensity,
      &m_global_voice_params->osc_fo_mod_range, TRANSFORM_NONE, false);
  matrix->AddMatrixRow(row);

  // LFO1 -> DEST_OSC3_OUTPUT_AMP
  row = CreateMatrixRow(SOURCE_LFO1, DEST_OSC3_OUTPUT_AMP,
                        &m_global_voice_params->lfo1_osc_mod_intensity,
                        &m_default_mod_range, TRANSFORM_BIPOLAR_TO_UNIPOLAR,
                        false);
  matrix->AddMatrixRow(row);

  // LFO1 -> DEST_OSC3_FO (VIBRATO)
  row = CreateMatrixRow(
      SOURCE_LFO1, DEST_OSC3_FO, &m_global_voice_params->lfo1_osc_mod_intensity,
      &m_global_voice_params->osc_fo_mod_range, TRANSFORM_NONE, false);
  matrix->AddMatrixRow(row);

  // LFO1 -> DEST_OSC4_OUTPUT_AMP
  row = CreateMatrixRow(SOURCE_LFO1, DEST_OSC4_OUTPUT_AMP,
                        &m_global_voice_params->lfo1_osc_mod_intensity,
                        &m_default_mod_range, TRANSFORM_BIPOLAR_TO_UNIPOLAR,
                        false);
  matrix->AddMatrixRow(row);

  // LFO1 -> DEST_OSC4_FO (VIBRATO)
  row = CreateMatrixRow(
      SOURCE_LFO1, DEST_OSC4_FO, &m_global_voice_params->lfo1_osc_mod_intensity,
      &m_global_voice_params->osc_fo_mod_range, TRANSFORM_NONE, false);
  matrix->AddMatrixRow(row);
}

void FMSynthVoice::SetLFO1Destination(unsigned int op, unsigned int dest) {
  switch (op) {
    case (0): {
      if (dest == DX_LFO_DEST_AMP_MOD &&
          m_op1.m_mod_source_amp != DEST_OSC1_OUTPUT_AMP) {
        m_op1.m_mod_source_amp = DEST_OSC1_OUTPUT_AMP;
        m_op1.m_mod_source_fo = DEST_NONE;
      } else if (dest == DX_LFO_DEST_VIBRATO &&
                 m_op1.m_mod_source_fo != DEST_OSC1_FO) {
        m_op1.m_mod_source_fo = DEST_OSC1_FO;
        m_op1.m_mod_source_amp = DEST_NONE;
      } else if (dest == DX_LFO_DEST_NONE &&
                 (m_op1.m_mod_source_amp != DEST_NONE ||
                  m_op1.m_mod_source_fo != DEST_NONE)) {
        m_op1.m_mod_source_amp = DEST_NONE;
        m_op1.m_mod_source_fo = DEST_NONE;
      }
      break;
    }
    case (1): {
      if (dest == DX_LFO_DEST_AMP_MOD &&
          m_op2.m_mod_source_amp != DEST_OSC2_OUTPUT_AMP) {
        m_op2.m_mod_source_amp = DEST_OSC2_OUTPUT_AMP;
        m_op2.m_mod_source_fo = DEST_NONE;
      } else if (dest == DX_LFO_DEST_VIBRATO &&
                 m_op2.m_mod_source_fo != DEST_OSC2_FO) {
        m_op2.m_mod_source_fo = DEST_OSC2_FO;
        m_op2.m_mod_source_amp = DEST_NONE;
      } else if (dest == DX_LFO_DEST_NONE &&
                 (m_op2.m_mod_source_amp != DEST_NONE ||
                  m_op2.m_mod_source_fo != DEST_NONE)) {
        m_op2.m_mod_source_amp = DEST_NONE;
        m_op2.m_mod_source_fo = DEST_NONE;
      }
      break;
    }
    case (2): {
      if (dest == DX_LFO_DEST_AMP_MOD &&
          m_op3.m_mod_source_amp != DEST_OSC3_OUTPUT_AMP) {
        m_op3.m_mod_source_amp = DEST_OSC3_OUTPUT_AMP;
        m_op3.m_mod_source_fo = DEST_NONE;
      } else if (dest == DX_LFO_DEST_VIBRATO &&
                 m_op3.m_mod_source_fo != DEST_OSC3_FO) {
        m_op3.m_mod_source_fo = DEST_OSC3_FO;
        m_op3.m_mod_source_amp = DEST_NONE;
      } else if (dest == DX_LFO_DEST_NONE &&
                 (m_op3.m_mod_source_amp != DEST_NONE ||
                  m_op3.m_mod_source_fo != DEST_NONE)) {
        m_op3.m_mod_source_amp = DEST_NONE;
        m_op3.m_mod_source_fo = DEST_NONE;
      }
      break;
    }
    case (3): {
      if (dest == DX_LFO_DEST_AMP_MOD &&
          m_op4.m_mod_source_amp != DEST_OSC4_OUTPUT_AMP) {
        m_op4.m_mod_source_amp = DEST_OSC4_OUTPUT_AMP;
        m_op4.m_mod_source_fo = DEST_NONE;
      } else if (dest == DX_LFO_DEST_VIBRATO &&
                 m_op4.m_mod_source_fo != DEST_OSC4_FO) {
        m_op4.m_mod_source_fo = DEST_OSC4_FO;
        m_op4.m_mod_source_amp = DEST_NONE;
      } else if (dest == DX_LFO_DEST_NONE &&
                 (m_op4.m_mod_source_amp != DEST_NONE ||
                  m_op4.m_mod_source_fo != DEST_NONE)) {
        m_op4.m_mod_source_amp = DEST_NONE;
        m_op4.m_mod_source_fo = DEST_NONE;
      }
      break;
    }
    default:
      break;
  }
}

void FMSynthVoice::PrepareForPlay() {
  Voice::PrepareForPlay();
  Reset();
}

void FMSynthVoice::Update() {
  if (!m_global_voice_params) return;

  Voice::Update();

  m_op1_feedback = m_global_voice_params->op1_feedback;
  m_op2_feedback = m_global_voice_params->op2_feedback;
  m_op3_feedback = m_global_voice_params->op3_feedback;
  m_op4_feedback = m_global_voice_params->op4_feedback;
}

void FMSynthVoice::Reset() {
  Voice::Reset();
  m_portamento_inc = 0.0;
}

bool FMSynthVoice::CanNoteOff() {
  bool ret = false;
  if (!m_note_on)
    return ret;
  else {
    switch (m_voice_mode) {
      case 0:
      case 1:
      case 2:
      case 3: {
        if (m_eg1.CanNoteOff()) ret = true;
        break;
      }
      case 4: {
        if (m_eg1.CanNoteOff() && m_eg2.CanNoteOff()) ret = true;
        break;
      }
      case 5:
      case 6: {
        if (m_eg1.CanNoteOff() && m_eg2.CanNoteOff() && m_eg3.CanNoteOff())
          ret = true;
        break;
      }
      case 7: {
        if (m_eg1.CanNoteOff() && m_eg2.CanNoteOff() && m_eg3.CanNoteOff() &&
            m_eg4.CanNoteOff())
          ret = true;
        break;
      }
    }
  }
  return ret;
}

bool FMSynthVoice::IsVoiceDone() {
  bool ret = false;
  switch (m_voice_mode) {
    case 0:
    case 1:
    case 2:
    case 3: {
      if (m_eg1.GetState() == OFFF) ret = true;
      break;
    }
    case 4: {
      if (m_eg1.GetState() == OFFF && m_eg2.GetState() == OFFF) ret = true;
      break;
    }
    case 5:
    case 6: {
      if (m_eg1.GetState() == OFFF && m_eg2.GetState() == OFFF &&
          m_eg3.GetState() == OFFF)
        ret = true;
      break;
    }
    case 7: {
      if (m_eg1.GetState() == OFFF && m_eg2.GetState() == OFFF &&
          m_eg3.GetState() == OFFF && m_eg4.GetState() == OFFF)
        ret = true;
      break;
    }
  }
  return ret;
}

void FMSynthVoice::SetOutputEGs() {
  m_eg1.m_output_eg = false;
  m_eg2.m_output_eg = false;
  m_eg3.m_output_eg = false;
  m_eg4.m_output_eg = false;

  switch (m_voice_mode) {
    case 0:
    case 1:
    case 2:
    case 3: {
      m_eg1.m_output_eg = true;
      break;
    }
    case 4: {
      m_eg1.m_output_eg = true;
      m_eg2.m_output_eg = true;
      break;
    }
    case 5:
    case 6: {
      m_eg1.m_output_eg = true;
      m_eg2.m_output_eg = true;
      m_eg3.m_output_eg = true;
      break;
    }
    case 7: {
      m_eg1.m_output_eg = true;
      m_eg2.m_output_eg = true;
      m_eg3.m_output_eg = true;
      m_eg4.m_output_eg = true;
      break;
    }
  }
}

bool FMSynthVoice::DoVoice(double *left_output, double *right_output) {
  if (!Voice::DoVoice(left_output, right_output)) {
    return false;
  }

  if (m_portamento_inc > 0.0 && m_op1.m_osc_fo != m_osc_pitch) {
    // target pitch has now been hit
    if (m_modulo_portamento >= 1.0) {
      m_modulo_portamento = 0.0;
      m_op1.m_osc_fo = m_osc_pitch;
      m_op2.m_osc_fo = m_osc_pitch;
      m_op3.m_osc_fo = m_osc_pitch;
      m_op4.m_osc_fo = m_osc_pitch;
    } else {
      // calculate the pitch multiplier for this sample interval
      double portamento_pitch_mult =
          pitch_shift_multiplier(m_modulo_portamento * m_portamento_semitones);

      // set it on our one and only one oscillator
      m_op1.m_osc_fo = m_portamento_start * portamento_pitch_mult;
      m_op2.m_osc_fo = m_portamento_start * portamento_pitch_mult;
      m_op3.m_osc_fo = m_portamento_start * portamento_pitch_mult;
      m_op4.m_osc_fo = m_portamento_start * portamento_pitch_mult;

      // inc the modulo
      m_modulo_portamento += m_portamento_inc;
    }

    // oscillator is still running, so just update
    m_op1.Update();
    m_op2.Update();
    m_op3.Update();
    m_op4.Update();
  }

  double out = 0.0;
  double out1, out2, out3, out4 = 0.0;
  double eg1, eg2, eg3, eg4 = 0.0;

  SetOutputEGs();

  modmatrix.DoModMatrix(0);

  m_eg1.Update();
  m_eg2.Update();
  m_eg3.Update();
  m_eg4.Update();

  eg1 = m_eg1.DoEnvelope(NULL);
  eg2 = m_eg2.DoEnvelope(NULL);
  eg3 = m_eg3.DoEnvelope(NULL);
  eg4 = m_eg4.DoEnvelope(NULL);

  m_lfo1.Update();
  m_lfo1.DoOscillate(NULL);

  ////// layer 1 //////////////////////////////
  modmatrix.DoModMatrix(1);

  Update();
  m_op1.Update();
  m_op2.Update();
  m_op3.Update();
  m_op4.Update();

  m_dca.Update();

  switch (m_voice_mode) {
    case (0): {
      out4 = IMAX * eg4 * m_op4.DoOscillate(NULL);

      m_op4.SetPhaseMod(out4 * m_op4_feedback);
      m_op4.Update();

      m_op3.SetPhaseMod(out4);
      m_op3.Update();

      out3 = IMAX * eg3 * m_op3.DoOscillate(NULL);

      m_op2.SetPhaseMod(out3);
      m_op2.Update();

      out2 = IMAX * eg2 * m_op2.DoOscillate(NULL);

      m_op1.SetPhaseMod(out2);
      m_op1.Update();

      out1 = IMAX * eg1 * m_op1.DoOscillate(NULL);

      out = out1;

      break;
    }
    case (1): {
      out4 = IMAX * eg4 * m_op4.DoOscillate(NULL);
      m_op4.SetPhaseMod(out4 * m_op4_feedback);
      m_op4.Update();

      out3 = IMAX * eg3 * m_op3.DoOscillate(NULL);

      m_op2.SetPhaseMod(out3 + out4);
      m_op2.Update();

      out2 = IMAX * eg2 * m_op2.DoOscillate(NULL);

      m_op1.SetPhaseMod(out2);
      m_op1.Update();

      out1 = IMAX * eg1 * m_op1.DoOscillate(NULL);

      out = out1;

      break;
    }
    case (2): {
      out4 = IMAX * eg4 * m_op4.DoOscillate(NULL);
      m_op4.SetPhaseMod(out4 * m_op4_feedback);
      m_op4.Update();

      out3 = IMAX * eg3 * m_op3.DoOscillate(NULL);

      m_op2.SetPhaseMod(out3);
      m_op2.Update();

      out2 = IMAX * eg2 * m_op2.DoOscillate(NULL);

      m_op1.SetPhaseMod(out2 + out4);
      m_op1.Update();

      out1 = IMAX * eg1 * m_op1.DoOscillate(NULL);

      out = out1;

      break;
    }
    case (3): {
      out4 = IMAX * eg4 * m_op4.DoOscillate(NULL);

      m_op4.SetPhaseMod(out4 * m_op4_feedback);
      m_op4.Update();

      m_op3.SetPhaseMod(out4);
      m_op3.Update();

      out3 = IMAX * eg3 * m_op3.DoOscillate(NULL);

      out2 = IMAX * eg2 * m_op2.DoOscillate(NULL);

      m_op1.SetPhaseMod(out2 + out3);
      m_op1.Update();

      out1 = IMAX * eg1 * m_op1.DoOscillate(NULL);

      out = out1;

      break;
    }
    case (4): {
      out4 = IMAX * eg4 * m_op4.DoOscillate(NULL);
      m_op4.SetPhaseMod(out4 * m_op4_feedback);
      m_op4.Update();

      out2 = IMAX * eg2 * m_op2.DoOscillate(NULL);

      m_op3.SetPhaseMod(out4);
      m_op3.Update();

      out3 = IMAX * eg3 * m_op3.DoOscillate(NULL);

      m_op1.SetPhaseMod(out2);
      m_op1.Update();

      out1 = IMAX * eg1 * m_op1.DoOscillate(NULL);

      out = 0.5 * out1 + 0.5 * out3;

      break;
    }
    case (5): {
      out4 = IMAX * eg4 * m_op4.DoOscillate(NULL);

      m_op4.SetPhaseMod(out4 * m_op4_feedback);
      m_op4.Update();

      m_op3.SetPhaseMod(out4);
      m_op3.Update();

      out3 = IMAX * eg3 * m_op3.DoOscillate(NULL);

      m_op2.SetPhaseMod(out4);
      m_op2.Update();

      out2 = IMAX * eg2 * m_op2.DoOscillate(NULL);

      m_op1.SetPhaseMod(out4);
      m_op1.Update();

      out1 = IMAX * eg1 * m_op1.DoOscillate(NULL);

      out = 0.33 * out1 + 0.33 * out2 + 0.33 * out3;

      break;
    }
    case (6): {
      out4 = IMAX * eg4 * m_op4.DoOscillate(NULL);
      m_op4.SetPhaseMod(out4 * m_op4_feedback);
      m_op4.Update();

      m_op3.SetPhaseMod(out4);
      m_op3.Update();

      out3 = IMAX * eg3 * m_op3.DoOscillate(NULL);

      out2 = IMAX * eg2 * m_op2.DoOscillate(NULL);

      out1 = IMAX * eg1 * m_op1.DoOscillate(NULL);

      out = 0.33 * out1 + 0.33 * out2 + 0.33 * out3;

      break;
    }
    case (7): {
      out4 = IMAX * eg4 * m_op4.DoOscillate(NULL);

      m_op4.SetPhaseMod(out4 * m_op4_feedback);
      m_op4.Update();

      out3 = IMAX * eg3 * m_op3.DoOscillate(NULL);
      out2 = IMAX * eg2 * m_op2.DoOscillate(NULL);
      out1 = IMAX * eg1 * m_op1.DoOscillate(NULL);

      out = 0.25 * out1 + 0.25 * out2 + 0.25 * out3 + 0.25 * out4;

      break;
    }
    default:
      break;
  }

  m_dca.DoDCA(out, out, left_output, right_output);

  return true;
}

void FMSynthVoice::SetSustainOverride(unsigned int op, bool b) {
  switch (op) {
    case 1:
      m_eg1.SetSustainOverride(b);
      break;
    case 2:
      m_eg2.SetSustainOverride(b);
      break;
    case 3:
      m_eg3.SetSustainOverride(b);
      break;
    case 4:
      m_eg4.SetSustainOverride(b);
      break;
  }
}
