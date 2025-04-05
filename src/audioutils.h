#pragma once

#include <defjams.h>

#include <string>

double pa_setup(void);
void pa_teardown(void);

struct AudioBufferDetails {
  std::string filename;
  int num_channels{0};
  int sample_rate{0};
  int buffer_length{0};
};

int get_chord_type(unsigned int scale_degree);
void get_midi_notes_from_chord(unsigned int note, unsigned int chord_type,
                               int octave, chord_midi_notes *chnotes);

int GetMidNumFromNote(std::string note);
std::string GetNoteFromMidiNum(int midi_num);
std::vector<int> GetMidiNotesInChord(unsigned int root_note,
                                     unsigned int chord_type,
                                     unsigned int modification = 0);
int GetThird(int midi_note, char key);
int GetFifth(int midi_note, char key);
int GetNthDegree(int midi_note, int degree, char key);
int GetStepsToNextDegree(int scale_index);
int GetScaleIndex(int note, char key);
int GetRootNote(int key, int index);
int Scale(int midi_note, char key);
bool IsMidiNoteInKeyChar(int midi_note, char key);
bool IsMidiNoteInKey(int midi_note, int key_midi_num);
bool IsNote(std::string input);
std::vector<int> ScaleMelodyToKey(const std::vector<int> &melody_in,
                                  int root_midi, int scale);

namespace audioutils {
// windowing functions.
double sinc(double x);
double han(double x, double window_len);
double blackman(double x, double window_len);

std::vector<double> resample(const std::vector<double> &input, int num_channels,
                             double pitch_ratio);
}  // namespace audioutils
