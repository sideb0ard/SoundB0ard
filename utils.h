#pragma once

#include "sbmsg.h"
#include <stdbool.h>

// void startrrr(int sig_num);
// void timed_sig_start(const char *sigtype, int freq, int freq2);
// void *algo_run(void *);

typedef struct freaky {
    int num_freaks;
    double *freaks;
} freaky;

void thrunner(SBMSG *msg);
void *timed_sig_start(void *arg);
void *fadeup_runrrr(void *arg);
void *fadedown_runrrr(void *arg);
void *duck_runrrr(void *arg);
void faderrr(int sig_num, direction d);
freaky *new_freqs_from_string(char *string);
float freqval(char *n);
int notelookup(char *n);
int ch_midi_lookup(int ch, void *fm);
void list_sample_dir(void);
void strim(const char *input, char *result);
void chordie(char *n);
int conv_bitz(int num);
int is_valid_osc(char *string);
void related_notes(char root_note[4], double *second_note, double *third_note);
double pitch_shift_multiplier(double pitch_shift_semitones);
void calculate_pan_values(double pan_total, double *pan_left,
                          double *pan_right);
double parabolic_sine(double x, bool highprecision);

// scales cur_val which is range of cur_min, cur_max, to be a new_val within
// new_min, new_max
double scaleybum(double cur_min, double cur_max, double new_min, double new_max,
                 double cur_val);

void itoa(int n, char s[]);
void reverse(char s[]);

double unipolar_to_bipolar(double value);

double convex_transform(double value);
double convex_inverted_transform(double value);
double concave_transform(double value);
double concave_inverted_transform(double value);

double do_pn_sequence(unsigned *pn_register);
double do_white_noise(void);
void check_wrap_index(double *index);
double do_blep_n(const double *blep_table, double table_len, double modulo,
                 double inc, double height, bool rising_edge,
                 double points_per_side, bool interpolate);

float lin_terp(float x1, float x2, float y1, float y2, float x);
void print_midi_event(int midi_num);
