#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <defjams.h>
typedef struct AbletonLink AbletonLink;

typedef struct LinkData {
    int num_peers;
    double quantum;
    double beat;
    double tempo;
    double phase;
} LinkData;

AbletonLink *new_ableton_link(double bpm);
void link_update_from_main_callback(AbletonLink *l, int num_frames);

void link_set_latency(AbletonLink *l, double latency);

LinkData link_get_timing_data_for_display(AbletonLink *l);

void update_bpm(double bpm);

int link_get_sample_time(AbletonLink *l);
double link_get_bpm(AbletonLink *l);
double link_get_beat_at_time(AbletonLink *l, long long int sample_number);
double link_get_phase_at_time(AbletonLink *l, long long int sample_number, int quantum);
double link_get_current_quantum(AbletonLink *l);

void link_update_mixer_timing_info(AbletonLink *l, mixer_timing_info *info, int frame_num);

void link_set_bpm(AbletonLink *l, double bpm);
void link_reset_beat_time(AbletonLink *l);

#ifdef __cplusplus
} // end extern "C"
#endif
