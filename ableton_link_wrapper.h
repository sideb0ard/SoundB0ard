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

AbletonLink *new_ableton_link(double bpm);
LinkData link_get_timing_data(AbletonLink *l);
void link_update_from_main_callback(AbletonLink *l, uint64_t host_time);

void update_bpm(double bpm);

void link_set_bpm(AbletonLink *l, double bpm);
void link_reset_beat_time(AbletonLink *l);

int link_get_sample_time(AbletonLink *l);
int link_get_samples_per_midi_tick(AbletonLink *l);
int link_get_loop_len_in_samples(AbletonLink *l);

double link_get_bpm(AbletonLink *l);
uint64_t link_get_host_time(AbletonLink *l);

#ifdef __cplusplus
} // end extern "C"
#endif
