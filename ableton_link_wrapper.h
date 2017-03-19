#pragma once

typedef struct AbletonLink AbletonLink;

#ifdef __cplusplus
extern "C" {
#endif

AbletonLink * new_ableton_link(void);
double get_bpm_from_link(AbletonLink *l);

#ifdef __cplusplus
} // end extern "C"
#endif
