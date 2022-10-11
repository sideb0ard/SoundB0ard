#include <synthfunctions.h>

#include <iostream>

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

std::map<std::string, double> GetPreset(int id, std::string preset_name) {
  std::map<std::string, double> preset_vals;

  std::cout << "YO, GET PRESET!" << preset_name << " for id:" << id << "\n";

  if (preset_name.empty()) {
    printf(
        "Play tha game, pal, need a name to LOAD yer synth settings "
        "with\n");
    return preset_vals;
  }

  const char *preset_to_load = preset_name.c_str();

  char line[4096];
  char setting_key[1024];
  char setting_val[1024];
  double scratch_val = 0.;

  FILE *presetzzz;

  if (id == DXSYNTH_TYPE) {
    presetzzz = fopen(DX_PRESET_FILENAME, "r+");
  } else if (id == MINISYNTH_TYPE) {
    presetzzz = fopen(MOOG_PRESET_FILENAME, "r+");
  } else {
    return preset_vals;
  }

  if (presetzzz == NULL) return preset_vals;

  char *tok, *last_tok;
  char const *sep = "::";

  while (fgets(line, sizeof(line), presetzzz)) {
    size_t n = strlen(line);
    if (line[n - 1] == '\n') line[n - 1] = '\0';

    int settings_count = 0;

    for (tok = strtok_r(line, sep, &last_tok); tok;
         tok = strtok_r(NULL, sep, &last_tok)) {
      sscanf(tok, "%[^=]=%s", setting_key, setting_val);
      sscanf(setting_val, "%lf", &scratch_val);
      if (strcmp(setting_key, "name") == 0) {
        if (strcmp(setting_val, preset_to_load) != 0) break;
      } else {
        preset_vals[setting_key] = scratch_val;
      }
    }
  }
  fclose(presetzzz);
  return preset_vals;
}
