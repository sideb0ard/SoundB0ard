
#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
namespace fs = std::filesystem;

#include "defjams.h"
#include "utils.h"
// #include "lookuptables.h"

static char const *rev_lookup[12] = {"c",  "c#", "d",  "d#", "e",  "f",
                                     "f#", "g",  "g#", "a",  "a#", "b"};

const double blep_table_center = 4096 / 2.0 - 1;

constexpr int kLocalBufferLen = 1024;

#define CONVEX_LIMIT 0.00398107
#define CONCAVE_LIMIT 0.99601893
#define ARC4RANDOMMAX 4294967295  // (2^32 - 1)
#define MAX_DIR_ENTRIES 512

#define EXTRACT_BITS(the_val, bits_start, bits_len) \
  ((the_val >> (bits_start - 1)) & ((1 << bits_len) - 1))

namespace {
void bjork(std::vector<std::vector<int>> &seqs,
           std::vector<std::vector<int>> &remainders) {
  if (remainders.size() < 2) return;
  int seq_insert_count = 0;
  auto r_it = remainders.begin();
  for (auto &s : seqs) {
    s.insert(s.end(), r_it->begin(), r_it->end());
    seq_insert_count++;
    r_it = remainders.erase(r_it);
    if (r_it == remainders.end()) {
      break;
    }
  }

  if (remainders.empty()) {
    std::copy(seqs.begin() + seq_insert_count, seqs.end(),
              std::back_inserter(remainders));
    seqs.erase(seqs.begin() + seq_insert_count, seqs.end());
  }

  if (remainders.size() > 1) bjork(seqs, remainders);
}

// void print_vec_of_vec(std::vector<std::vector<int>> &vec) {
//   for (auto s : vec) {
//     for (auto n : s) std::cout << n;
//     std::cout << std::endl;
//   }
// }
}  // namespace

namespace utils {

bool FileExists(std::string filename) {
  fs::path file_path = fs::current_path().string() + "/wavs/" + filename;
  return fs::exists(file_path);
}

float LinTerp(float x1, float x2, float y1, float y2, float x) {
  float denom = x2 - x1;
  if (denom == 0) return y1;  // should not ever happen

  // calculate decimal position of x
  float dx = (x - x1) / (x2 - x1);

  // use weighted sum method of interpolating
  float result = dx * y2 + (1 - dx) * y1;

  return result;
}
}  // namespace utils

std::string GetRandomSampleNameFromDir(std::string sample_dir) {
  std::vector<std::string> file_names;
  std::string dir_prefix = "wavs/";
  for (const auto &entry : fs::directory_iterator(dir_prefix + sample_dir)) {
    auto fpath = entry.path();
    if (fpath.compare(".DS_Store") != 0) file_names.push_back(fpath);
  }

  return file_names[rand() % file_names.size()].substr(dir_prefix.length());
}

// static void qsort_char_array(char **wurds, int lower_idx, int upper_idx)
//{
//    if (lower_idx >= upper_idx)
//        return;
//    int middle_idx = lower_idx;
//    for (int i = lower_idx + 1; i < upper_idx; i++)
//        if (!first_wurd_before_second(wurds[i], wurds[lower_idx]))
//            switch_wurds(wurds, ++middle_idx, i);
//    switch_wurds(wurds, lower_idx, middle_idx);
//    qsort_char_array(wurds, lower_idx, middle_idx - 1);
//    qsort_char_array(wurds, middle_idx + 1, upper_idx);
//}

std::string list_sample_dir(std::string indir) {
  std::stringstream ss;
  std::vector<std::string> listing{};
  std::string dirpath = ".";
  dirpath.append(SAMPLE_DIR);
  dirpath += indir;
  try {
    for (const auto &p : std::filesystem::directory_iterator(dirpath)) {
      auto pathname = p.path().string();
      pathname.erase(0, 7);
      listing.push_back(pathname);
    }
  } catch (std::filesystem::filesystem_error e) {
    std::cerr << "nope.\n";
  }

  std::sort(listing.begin(), listing.end());
  for (auto l : listing) ss << l << "\n";

  return ss.str();
}

// void related_notes(char note[4], double *second_note, double *third_note) {
//   char root_note;
//   int scale;
//   sscanf(note, "%[a-z#]%d", &root_note, &scale);
//
//   char scale_ch[2];
//   sprintf(scale_ch, "%d", scale);
//   int second_note_num = (root_note + 4) % 12;
//   int third_note_num = (root_note + 3) % 12;
//
//   char sec_note[4];
//   char thr_note[4];
//
//   strcpy(sec_note, rev_lookup[second_note_num]);
//   strcpy(thr_note, rev_lookup[third_note_num]);
//
//   strcat(sec_note, scale_ch);
//   strcat(thr_note, scale_ch);
//   *second_note = freqval(sec_note);
//   *third_note = freqval(thr_note);
// }

int notelookup(char *n) {
  // twelve semitones:
  // C C#/Db D D#/Eb E F F#/Gb G G#/Ab A A#/Bb B
  //
  if (!strcasecmp("c", n))
    return 0;
  else if (!strcasecmp("c#", n))
    return 1;
  else if (!strcasecmp("db", n))
    return 1;
  else if (!strcasecmp("d", n))
    return 2;
  else if (!strcasecmp("d#", n))
    return 3;
  else if (!strcasecmp("eb", n))
    return 3;
  else if (!strcasecmp("e", n))
    return 4;
  else if (!strcasecmp("f", n))
    return 5;
  else if (!strcasecmp("f#", n))
    return 6;
  else if (!strcasecmp("gb", n))
    return 6;
  else if (!strcasecmp("g", n))
    return 7;
  else if (!strcasecmp("g#", n))
    return 8;
  else if (!strcasecmp("ab", n))
    return 8;
  else if (!strcasecmp("a", n))
    return 9;
  else if (!strcasecmp("a#", n))
    return 10;
  else if (!strcasecmp("bb", n))
    return 10;
  else if (!strcasecmp("b", n))
    return 11;
  else
    return -1;
}

// float chfreqlookup(int ch, void *p)
// int ch_midi_lookup(int ch, int octave, char *keytext)
int input_key_to_char_note(int ch, int octave, char *keytext) {
  // numbers from
  // http://www.inspiredacoustics.com/en/MIDI_note_numbers_and_center_frequencies
  int cur_octave_midi_num = 12 + (octave * 12);
  int next_octave = cur_octave_midi_num + 12;

  int midi_num = -1;

  switch (ch) {
    case 97:                               // C - a
      midi_num = 0 + cur_octave_midi_num;  // C
      strncpy(keytext, "C", 1);
      break;
    case 119:
      midi_num = 1 + cur_octave_midi_num;  // C#
      strncpy(keytext, "C#", 2);
      break;
    case 115:
      midi_num = 2 + cur_octave_midi_num;  // D
      strncpy(keytext, "D", 1);
      break;
    case 101:
      midi_num = 3 + cur_octave_midi_num;  // D#
      strncpy(keytext, "D#", 2);
      break;
    case 100:
      midi_num = 4 + cur_octave_midi_num;  // E
      strncpy(keytext, "E", 1);
      break;
    case 102:
      midi_num = 5 + cur_octave_midi_num;  // F
      strncpy(keytext, "F", 1);
      break;
    case 116:
      midi_num = 6 + cur_octave_midi_num;  // F#
      strncpy(keytext, "F#", 2);
      break;
    case 103:
      midi_num = 7 + cur_octave_midi_num;  // G
      strncpy(keytext, "G", 1);
      break;
    case 121:
      midi_num = 8 + cur_octave_midi_num;  // G#
      strncpy(keytext, "G#", 2);
      break;
    case 104:
      midi_num = 9 + cur_octave_midi_num;  // A
      strncpy(keytext, "A", 1);
      break;
    case 117:
      midi_num = 10 + cur_octave_midi_num;  // A#
      strncpy(keytext, "A#", 2);
      break;
    case 106:
      midi_num = 11 + cur_octave_midi_num;  // B
      strncpy(keytext, "B", 1);
      break;
    case 107:
      midi_num = 0 + next_octave;  // C
      strncpy(keytext, "C", 1);
      break;
    case 111:
      midi_num = 1 + next_octave;  // C#
      strncpy(keytext, "C#", 2);
      break;
    case 108:
      midi_num = 2 + next_octave;  // D
      strncpy(keytext, "D", 1);
      break;
      // default:
      //    midi_num = -1;
  }
  return midi_num;
}

void strim(const char *input, char *result) {
  int flag = 0;

  while (*input) {
    if (!isspace((unsigned char)*input) && flag == 0) {
      *result++ = *input;
      flag = 1;
    }
    input++;
    if (flag == 1) {
      *result++ = *input;
    }
  }

  while (1) {
    result--;
    if (!isspace((unsigned char)*input) && flag == 0) {
      break;
    }
    flag = 0;
    *result = '\0';
  }
}

int conv_bitz(int num) {
  for (int i = 0; i < 16; i++) {
    if ((num & (1 << i)) == num) {
      // printf("%d is %d\n", num, i);
      return i;
    }
  }
  return -1;
}

bool IsValidFile(std::string filename) {
  return fs::exists(fs::current_path().string() + "/wavs/" + filename);
}

// from
// https://martin.ankerl.com/2012/01/25/optimized-approximative-pow-in-c-and-cpp/
double fast_pow(double a, double b) {
  union {
    double d;
    int x[2];
  } u = {a};
  u.x[1] = (int)(b * (u.x[1] - 1072632447) + 1072632447);
  u.x[0] = 0;
  return u.d;
}

double pitch_shift_multiplier(double pitch_shift_semitones)
// from Will Pirkle book 'Designing Software Synthesizer Plug-Ins...'
{
  if (pitch_shift_semitones == 0) return 1.0;
  // return pow(2.0, pitch_shift_semitones / 12.0);
  return fast_pow(2.0, pitch_shift_semitones / 12.0);
}

void calculate_pan_values(double pan_total, double *pan_left,
                          double *pan_right) {
  *pan_left = cos((M_PI / 4.0) * (pan_total + 1.0));
  *pan_right = sin((M_PI / 4.0) * (pan_total + 1.0));

  *pan_left = fmax(*pan_left, (double)0.0);
  *pan_left = fmin(*pan_left, (double)1.0);

  *pan_right = fmax(*pan_right, (double)0.0);
  *pan_right = fmin(*pan_right, (double)1.0);
}

// From K&R C - cut n pasted from
// https://en.wikibooks.org/wiki/C_Programming/C_Reference/stdlib.h/itoa
/* itoa:  convert n to characters in s */
void itoa(int n, char s[]) {
  int i, sign;

  if ((sign = n) < 0) /* record sign */
    n = -n;           /* make n positive */
  i = 0;
  do {                     /* generate digits in reverse order */
    s[i++] = n % 10 + '0'; /* get next digit */
  } while ((n /= 10) > 0); /* delete it */
  if (sign < 0) s[i++] = '-';
  s[i] = '\0';
  reverse_string(s);
}

/* reverse:  reverse string s in place */
void reverse_string(char s[]) {
  int i, j;
  char c;

  for (i = 0, j = strlen(s) - 1; i < j; i++, j--) {
    c = s[i];
    s[i] = s[j];
    s[j] = c;
  }
}

// scales cur_val which is range of cur_min, cur_max, to be a new_val within
// new_min, new_max
// algo from
// http://stackoverflow.com/questions/929103/convert-a-number-range-to-another-range-maintaining-ratio
double scaleybum(double cur_min, double cur_max, double new_min, double new_max,
                 double cur_val) {
  double return_val = 0;

  double cur_range = cur_max - cur_min;
  if (cur_range == 0)
    return_val = new_min;
  else {
    double new_range = new_max - new_min;
    return_val = (((cur_val - cur_min) * new_range) / cur_range) + new_min;
  }

  return return_val;
}
// hmm, this now seems to make more sense for ordering than scaleybum!
double scale(double cur_val, double cur_min, double cur_max, double new_min,
             double new_max) {
  return scaleybum(cur_min, cur_max, new_min, new_max, cur_val);
}

double wrap(double cur_val, double min_val, double max_val) {
  double cur_range = max_val - min_val;
  while (cur_val > max_val) {
    cur_val -= cur_range;
  }
  return cur_val;
}

double fold(double cur_val, double min_val, double max_val) {
  int attempts = 0;
  while (cur_val < min_val || cur_val > max_val) {
    if (cur_val < min_val) {
      double diff = min_val - cur_val;
      cur_val = min_val + diff;

    } else if (cur_val > max_val) {
      double diff = cur_val - max_val;
      cur_val = max_val - diff;
    }
    attempts++;
    if (attempts > 5) break;
  }
  return cur_val;
}

// http://devmaster.net/posts/9648/fast-and-accurate-sine-cosine
// input is -pi to +pi
// i took it from the Will Pirkle Synth book.
double parabolic_sine(double x, bool high_precision) {
  const double B = 4.0 / (float)M_PI;
  const double C = -4.0 / ((float)M_PI * (float)M_PI);
  const double P = 0.225;
  double y = B * x + C * x * fabs(x);

  if (high_precision) y = P * (y * fabs(y) - y) + y;
  return y;
}

double unipolar_to_bipolar(double value) { return 2.0 * value - 1.0; }
double bipolar_to_unipolar(double value) { return 0.5 * value + 0.5; }

double convex_transform(double value) {
  if (value <= CONVEX_LIMIT) return 0.0;

  return 1.0 + (5.0 / 12.0) * log10(value);
}

/* convexInvertedTransform()

        calculates the convexInvertedTransform of the input

        value = the unipolar (0 -> 1) value to convert
*/
double convex_inverted_transform(double value) {
  if (value >= CONCAVE_LIMIT) return 0.0;

  return 1.0 + (5.0 / 12.0) * log10(1.0 - value);
}

/* concaveTransform()

        calculates the concaveTransform of the input

        value = the unipolar (0 -> 1) value to convert
*/
double concave_transform(double value) {
  if (value >= CONCAVE_LIMIT) return 1.0;

  return -(5.0 / 12.0) * log10(1.0 - value);
}

/* concaveInvertedTransform()

        calculates the concaveInvertedTransform of the input

        value = the unipolar (0 -> 1) value to convert
*/
double concave_inverted_transform(double value) {
  if (value <= CONVEX_LIMIT) return 1.0;

  return -(5.0 / 12.0) * log10(value);
}

double do_white_noise() {
  float noise = 0.0;

  // fNoise is 0 -> ARC4RANDOMMAX
  noise = (float)arc4random();

  // normalize and make bipolar
  noise = 2.0 * (noise / ARC4RANDOMMAX) - 1.0;

  return noise;
}

double do_pn_sequence(unsigned *pn_register) {
  // get the bits
  unsigned b0 =
      EXTRACT_BITS(*pn_register, 1, 1);  // 1 = b0 is FIRST bit from right
  unsigned b1 =
      EXTRACT_BITS(*pn_register, 2, 1);  // 1 = b1 is SECOND bit from right
  unsigned b27 = EXTRACT_BITS(*pn_register, 28,
                              1);  // 28 = b27 is 28th bit from right
  unsigned b28 = EXTRACT_BITS(*pn_register, 29,
                              1);  // 29 = b28 is 29th bit from right

  // form the XOR
  unsigned b31 = b0 ^ b1 ^ b27 ^ b28;

  // form the mask to OR with the register to load b31
  if (b31 == 1) b31 = 0x10000000;

  // shift one bit to right
  *pn_register >>= 1;

  // set the b31 bit
  *pn_register |= b31;

  // convert the output into a floating point number, scaled by
  // experimentation
  // to a range of o to +2.0
  float out = (float)(*pn_register) / ((pow((float)2.0, (float)32.0)) / 16.0);

  // shift down to form a result from -1.0 to +1.0
  out -= 1.0;

  return out;
}

void CheckWrapIndex(double *index) {
  while (*index < 0.0) *index += 1.0;

  while (*index >= 1.0) *index -= 1.0;
}

double do_blep_n(const double *blep_table, double table_len, double modulo,
                 double inc, double height, bool rising_edge,
                 double points_per_side, bool interpolate) {
  double blep = 0.0;
  double t = 0.0;

  for (int i = 1; i <= (int)points_per_side; i++) {
    if (modulo > 1.0 - (double)i * inc) {
      t = (modulo - 1.0) / (points_per_side * inc);
      float idx = (1.0 + t) * blep_table_center;

      if (interpolate) {
        blep = blep_table[(int)idx];
      } else {
        float idx = (1.0 + t) * blep_table_center;
        float frac = idx - (int)idx;
        blep = utils::LinTerp(0, 1, blep_table[(int)idx],
                              blep_table[(int)idx + 1], frac);
      }

      if (!rising_edge) blep *= -1.0;

      return height * blep;
    }
  }

  for (int i = 1; i <= (int)points_per_side; i++) {
    if (modulo < (double)i * inc) {
      t = modulo / (points_per_side * inc);
      float idx = t * blep_table_center + (blep_table_center + 1.0);

      if (interpolate)
        blep = blep_table[(int)idx];
      else {
        float frac = idx - (int)idx;
        if ((int)idx + 1 >= table_len)
          blep =
              utils::LinTerp(0, 1, blep_table[(int)idx], blep_table[0], frac);
        else
          blep = utils::LinTerp(0, 1, blep_table[(int)idx],
                                blep_table[(int)idx + 1], frac);
      }

      if (!rising_edge) blep *= -1.0;

      return height * blep;
    }
  }

  return 0.0;
}

float fasttan(float x) {
  static const float halfpi = 1.5707963267948966f;
  return sin(x) / sin(x + halfpi);
}

float fasttanh(float p) { return p / (fabs(2 * p) + 3 / (2 + 2 * p * 2 * p)); }

// no idea! taken from Will Pirkle books
float fastlog2(float x) {
  union {
    float f;
    unsigned int i;
  } vx = {x};
  union {
    unsigned int i;
    float f;
  } mx = {(vx.i & 0x007FFFFF) | 0x3f000000};
  float y = vx.i;
  y *= 1.1920928955078125e-7f;

  return y - 124.22551499f - 1.498030302f * mx.f -
         1.72587999f / (0.3520887068f + mx.f);
}

double semitones_between_frequencies(double start_freq, double end_freq) {
  return fastlog2(end_freq / start_freq) * 12.0;
}

double mma_midi_to_atten(unsigned int midi_val) {
  if (midi_val == 0) return 0.0;  // floor
  return ((double)midi_val * (double)midi_val) / (127.0 * 127.0);
}

double mma_midi_to_atten_db(unsigned int midi_val) {
  if (midi_val == 0) return -96.0;  // dB floor
  return 20.0 * log10((127.0 * 127.0) / ((float)midi_val * (float)midi_val));
}

bool is_int_member_in_array(int member_to_look_for, int *array_to_look_in,
                            int size_of_array) {
  for (int i = 0; i < size_of_array; i++) {
    if (array_to_look_in[i] == member_to_look_for) return true;
  }
  return false;
}

/*  //// FROM will pirkle book - http://www.willpirkle.com/
    Function:   lagrpol() implements n-order Lagrange Interpolation

    Inputs:     double* x   Pointer to an array containing the x-coordinates
   of the input values double* y   Pointer to an array containing the
   y-coordinates of the input values int n       The order of the
   interpolator, this is also the length of the x,y input arrays double xbar
   The x-coorinates whose y-value we want to interpolate

    Returns     The interpolated value y at xbar. xbar ideally is between
   the middle two values in the input array, but can be anywhere within the
   limits, which is needed for interpolating the first few or last few
   samples in a table with a fixed size.
*/
double lagrpol(double *x, double *y, int n, double xbar) {
  int i, j;
  double fx = 0.0;
  double l = 1.0;
  for (i = 0; i < n; i++) {
    l = 1.0;
    for (j = 0; j < n; j++) {
      if (j != i) l *= (xbar - x[j]) / (x[i] - x[j]);
    }
    fx += l * y[i];
  }
  return (fx);
}

double clamp(double min, double max, double v) {
  if (v < min) return min;
  if (v > max) return max;
  return v;
}

void print_bin_num(int num) {
  int len = 16;  // short
  for (int i = len - 1; i >= 0; --i) {
    if (num & 1 << i)
      printf("1");
    else
      printf("0");
  }
  printf("\n");
}

std::string bin_num_to_string(uint16_t num) {
  std::stringstream ss;
  int len = 16;  // short
  for (int i = len - 1; i >= 0; --i) {
    if (num & 1 << i)
      ss << "1";
    else
      ss << "0";
  }
  return ss.str();
}

int how_many_bits_in_num(unsigned int num) {
  printf("My num is %u\n", num);
  int len = sizeof(num) * 8;
  printf("Size of int:%lu len:%d\n", sizeof(num), len);
  int count = 0;
  for (uint32_t i = len; i > 0; --i) {
    if (num & (1 << i)) {
      count++;
      printf("Position %d\n", i);
    }
  }
  return count;
}

std::string get_string_logo() {
  std::stringstream ss;
  ss <<
      // clang-format off
        COOL_COLOR_GREEN
        // style is 'doom' from http://patorjk.com/software/taag/
        << "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n"
        << COOL_COLOR_YELLOW
        << "             _____                       _ _     _____               _\n"
        << "            /  ___|                     | | |   |  _  |             | |\n"
        << "            \\ `--.  ___  _   _ _ __   __| | |__ | |/\" | __ _ _ __ __| |\n"
        << "             `--. \\/ _ \\| | | | \"_ \\ / _` | \"_ \\|  /| |/ _` | \"__/ _` |\n"
        << "            /\\__/ / (_) | |_| | | | | (_| | |_) \\ |_/ / (_| | | | (_| |\n"
        << "            \\____/ \\___/ \\__,_|_| |_|\\__,_|_.__/ \\___/ \\__,_|_|  \\__,_|\n"
        << COOL_COLOR_GREEN
        << "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n"
        << ANSI_COLOR_RESET;
  // clang-format on
  return ss.str();
}

uint16_t mask_from_string(char *stringey_mask) {
  uint16_t bin_mask = 0;
  if (!stringey_mask) return bin_mask;

  int mask_len = strnlen(stringey_mask, 17);
  if (mask_len > 15) {
    for (int i = 0; i < 16; i++)
      if (stringey_mask[i] == '1') bin_mask |= 1 << (15 - i);
  } else if (mask_len > 3) {
    for (int i = 0; i < 4; i++) {
      int shift_amount = (15 - (i * 4));
      int bin_rep = 0;
      if (stringey_mask[i] == 'f')
        bin_rep = 0b1111;
      else if (stringey_mask[i] == 'e')
        bin_rep = 0b1110;
      else if (stringey_mask[i] == 'd')
        bin_rep = 0b1101;
      else if (stringey_mask[i] == 'c')
        bin_rep = 0b1100;
      else if (stringey_mask[i] == 'b')
        bin_rep = 0b1011;
      else if (stringey_mask[i] == 'a')
        bin_rep = 0b1010;
      else
        bin_rep = stringey_mask[i] - '0';

      for (int j = 0; j < 4; j++)
        if (bin_rep & 1 << (3 - j)) {
          bin_mask |= 1 << (shift_amount - j);
        }
    }
  }

  return bin_mask;
}

bool IsDigit(char c) { return '0' <= c && c <= '9'; }

bool IsValidIdentifier(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_' ||
         c == '-' || c == '.' || IsDigit(c);
}

bool IsBalanced(std::string &input) {
  // dumb algorithm counting matching number of curly braces.
  int num_braces = 0;
  const int len = input.length();
  for (int i = 0; i < len; i++) {
    if (input[i] == '{')
      num_braces++;
    else if (input[i] == '}')
      num_braces--;
  }

  return num_braces == 0;
}
void str_upper(std::string &data) {
  std::for_each(data.begin(), data.end(), [](char &c) { c = ::toupper(c); });
}

void str_lower(std::string &data) {
  std::for_each(data.begin(), data.end(), [](char &c) { c = ::tolower(c); });
}

std::vector<int> GenerateBjork(int num_pulses, int seq_length) {
  if (!num_pulses) return std::vector(seq_length, 0);

  std::vector<std::vector<int>> _seqs;
  for (int i = 0; i < num_pulses; i++) _seqs.push_back(std::vector<int>(1, 1));
  std::vector<std::vector<int>> _remainder;
  for (int i = 0; i < seq_length - num_pulses; i++)
    _remainder.push_back(std::vector<int>(1, 0));

  // print_vec_of_vec(_seqs);
  // print_vec_of_vec(_remainder);

  bjork(_seqs, _remainder);

  std::vector<int> bjorks;
  for (auto s : _seqs) {
    for (auto n : s) bjorks.push_back(n);
  }
  for (auto r : _remainder) {
    for (auto n : r) bjorks.push_back(n);
  }

  return bjorks;
}

std::vector<std::string> GetSynthPresets(unsigned int synthtype) {
  std::cout << "GET ME!\n";

  std::string preset_file_name{};
  switch (synthtype) {
    case (MINISYNTH_TYPE):
      preset_file_name = MOOG_PRESET_FILENAME;
      break;
    case (DXSYNTH_TYPE):
      preset_file_name = DX_PRESET_FILENAME;
      break;
  }

  std::vector<std::string> preset_names{};
  std::ifstream presetzzz{preset_file_name};

  if (presetzzz.is_open()) {
    std::string line;

    while (std::getline(presetzzz, line)) {
      std::string delimiter = "::";

      size_t pos = 0;
      std::string cur_setting{};
      std::string name{};
      std::string val{};

      while ((pos = line.find(delimiter)) != std::string::npos) {
        cur_setting = line.substr(0, pos);
        std::stringstream ss{cur_setting};
        int found_count{0};
        while (!ss.eof()) {
          if (found_count == 0)
            std::getline(ss, name, '=');
          else if (found_count == 1)
            std::getline(ss, val, '=');
          found_count++;
        }

        if (name == "name") {
          preset_names.push_back(val);
          break;
        }
        line.erase(0, pos + delimiter.length());
      }
    }
  }
  return preset_names;
}
