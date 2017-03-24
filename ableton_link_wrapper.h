#pragma once

typedef struct AbletonLink AbletonLink;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LinkData {
    int num_peers;
    double quantum;
    double beat;
    double tempo;
    double phase;
} LinkData;

typedef struct link_callback_timing_data {
    // data is relative to a given quantum
    double this_quantum;
    double beat_at_time;
    double phase_this_sample;
    double phase_last_sample;
} link_callback_timing_data;

AbletonLink *new_ableton_link(double bpm);
void link_update_from_main_callback(AbletonLink *l);

LinkData link_get_timing_data(AbletonLink *l);
link_callback_timing_data link_get_callback_timing_data(AbletonLink *l, double quantum);

void update_bpm(double bpm);

bool link_is_start_of_sixteenth(AbletonLink *l, int sample_num, int *sx_tick);

int link_get_sample_time(AbletonLink *l);
int link_get_samples_per_midi_tick(AbletonLink *l);
int link_get_loop_len_in_samples(AbletonLink *l);
double link_get_bpm(AbletonLink *l);
double link_get_beat_at_time(AbletonLink *l, int sample_number);
double link_get_current_quantum(AbletonLink *l);

void link_set_bpm(AbletonLink *l, double bpm);
void link_reset_beat_time(AbletonLink *l);

#ifdef __cplusplus
} // end extern "C"
#endif
