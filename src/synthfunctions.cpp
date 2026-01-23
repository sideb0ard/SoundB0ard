#include <synthfunctions.h>

#include <fstream>
#include <iostream>
#include <sstream>

namespace {
std::vector<std::string> tokenize(std::string const &str, std::string delim) {
  size_t start;
  size_t end = 0;

  std::vector<std::string> tokens;

  while ((start = str.find_first_not_of(delim, end)) != std::string::npos) {
    end = str.find(delim, start);
    tokens.push_back(str.substr(start, end - start));
  }
  return tokens;
}

}  // namespace

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

  std::ifstream preset_file;
  if (preset_name.empty()) {
    printf(
        "Play tha game, pal, need a name to LOAD yer synth settings "
        "with\n");
    return preset_vals;
  }

  if (id == FMSYNTH_TYPE)
    preset_file.open(FM_PRESET_FILENAME, std::ifstream::in);
  else if (id == MINISYNTH_TYPE)
    preset_file.open(MOOG_PRESET_FILENAME, std::ifstream::in);
  else if (id == DRUMSYNTH_TYPE)
    preset_file.open(DRUM_PRESET_FILENAME, std::ifstream::in);

  if (!preset_file.is_open()) {
    std::cerr << "WOOF, COULDNT OPEN " << preset_name << std::endl;
    return preset_vals;
  }

  bool found_preset{false};
  std::string line;
  std::string field_delim{"::"};

  while (getline(preset_file, line) && !found_preset) {
    std::vector<std::string> tokens = tokenize(line, field_delim);

    for (const auto &t : tokens) {
      std::stringstream ss{t};
      std::string key;
      std::string val;

      std::string tmp;
      int tokecount = 0;
      while (getline(ss, tmp, '=')) {
        if (tokecount == 0) {
          key = tmp;
        } else {
          val = tmp;
        }
        tokecount++;
      }

      if (tokecount == 2) {
        if (key == "name" && val == preset_name) {
          found_preset = true;
        }
      }
      if (!found_preset) continue;

      try {
        double dval = std::stod(val);
        preset_vals[key] = dval;
      } catch (std::invalid_argument) {
        // no-op - ignore name which is a string
      }
    }
  }

  preset_file.close();
  return preset_vals;
}
