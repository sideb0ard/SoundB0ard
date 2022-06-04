#pragma once

#include <audioutils.h>
#include <defjams.h>
#include <stdbool.h>
#include <stdint.h>

#include <string>
#include <vector>

void get_full_filename(char *basename, char *destination_fullname);

AudioBufferDetails ImportFileContents(std::vector<double> &buffer,
                                      std::string filename);
float freqval(char *n);
int notelookup(char *n);
int input_key_to_char_note(int ch, int octave, char *keytext);
int char_midi_lookup(char *char_midi);
std::string list_sample_dir(std::string indir = "");
std::string GetRandomSampleNameFromDir(std::string sample_dir);
void strim(const char *input, char *result);
void chordie(char *n);
int conv_bitz(int num);
bool IsValidFile(std::string filename);
void related_notes(char root_note[4], double *second_note, double *third_note);
double pitch_shift_multiplier(double pitch_shift_semitones);
double fast_pow(double a, double b);
void calculate_pan_values(double pan_total, double *pan_left,
                          double *pan_right);
double parabolic_sine(double x, bool highprecision);

// scales cur_val which is range of cur_min, cur_max, to be a new_val within
// new_min, new_max
double scaleybum(double cur_min, double cur_max, double new_min, double new_max,
                 double cur_val);

void itoa(int n, char s[]);
void reverse_string(char s[]);
void str_upper(std::string &input);
void str_lower(std::string &input);

double unipolar_to_bipolar(double value);
double bipolar_to_unipolar(double value);

double convex_transform(double value);
double convex_inverted_transform(double value);
double concave_transform(double value);
double concave_inverted_transform(double value);

double do_pn_sequence(unsigned *pn_register);
double do_white_noise(void);
void CheckWrapIndex(double *index);
double do_blep_n(const double *blep_table, double table_len, double modulo,
                 double inc, double height, bool rising_edge,
                 double points_per_side, bool interpolate);

void print_midi_event(int midi_num);
void print_bin_num(int num);
std::string bin_num_to_string(uint16_t num);
int how_many_bits_in_num(unsigned int num);
float fasttan(float x);
float fasttanh(float x);
float fastlog2(float x);
float fastpow2(float x);
float fastexp(float x);
double lagrpol(double *x, double *y, int n, double xbar);
double semitones_between_frequencies(double start_freq, double end_freq);
double clamp(double min, double max, double v);

double mma_midi_to_atten(unsigned int midi_val);
double mma_midi_to_atten_db(unsigned int midi_val);
bool is_int_member_in_array(int member_to_look_for, int *array_to_look_in,
                            int size_of_array);
unsigned int get_next_compat_note(unsigned int cur_key);
std::string get_string_logo(void);

bool IsDigit(char c);
bool IsValidIdentifier(char c);
bool IsBalanced(std::string &input);

std::vector<int> GenerateBjork(int num_pulses, int seq_length);

namespace utils {
float LinTerp(float x1, float x2, float y1, float y2, float x);
bool FileExists(std::string filename);
}  // namespace utils

bool synth_list_presets(unsigned int synthtype);
std::vector<std::string> synth_return_presets(unsigned int synthtype);
void PrintMultiMidi(MultiEventMidiPattern &pattern);
std::string MultiMidiString(MultiEventMidiPattern &pattern);
