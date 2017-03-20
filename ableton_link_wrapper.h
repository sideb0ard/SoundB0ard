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
void bump_bpm(double bpm);
void link_set_bpm(AbletonLink *l, double bpm);
void link_update_from_main_callback(AbletonLink *l);

#ifdef __cplusplus
} // end extern "C"
#endif
