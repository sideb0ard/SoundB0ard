#include <synthfunctions.h>

double midi_to_pan_value(unsigned int midi_val) {
  // see MMA DLS Level 2 Spec; controls are asymmetrical
  if (midi_val == 64)
    return 0.0;
  else if (midi_val <= 1)  // 0 or 1
    return -1.0;

  return 2.0 * (double)midi_val / 127.0 - 1.0;
}

double mma_midi_to_atten_dB(unsigned int midi_val) {
  if (midi_val == 0) return -96.0;  // dB floor

  return 20.0 * log10((127.0 * 127.0) / ((float)midi_val * (float)midi_val));
}

double midi_to_bipolar(unsigned int midi_val) {
  return 2.0 * (double)midi_val / 127.0 - 1.0;
}

double calculate_dx_amp(double dx_level) {
  // algo all from Will Pirkle
  double dx_amp = 0.0;
  if (dx_level != 0.0) {
    dx_amp = dx_level;
    dx_amp -= 99.0;
    dx_amp /= 1.32;

    dx_amp = (pow(10.0, dx_amp / 20.0));
  }
  return dx_amp;
}
