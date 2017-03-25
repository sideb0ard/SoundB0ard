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
    bool is_midi_tick;
    bool is_start_of_loop;
    bool is_start_of_quarter;
    bool is_start_of_sixteenth;
    int  sx_tick; // used in sequencer to keep sixteenth ticks in sync
} link_callback_timing_data;

AbletonLink *new_ableton_link(double bpm);
void link_update_from_main_callback(AbletonLink *l);

LinkData link_get_timing_data_for_display(AbletonLink *l);
link_callback_timing_data link_get_callback_timing_data(AbletonLink *l, int sample_num);

void update_bpm(double bpm);

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
