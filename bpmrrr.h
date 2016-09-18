#pragma once

typedef struct t_bpmrrr {
    int bpm;
    int cur_tick;
    int sixteenth_note_tick;
    double sleeptime;
} bpmrrr;

bpmrrr *new_bpmrrr(void);
void bpm_change(bpmrrr *b, int bpm);
void *bpm_run(void *b);
void bpm_info(bpmrrr *b);
void bpm_inc_tick(bpmrrr *b);
int bpm_get_tick(bpmrrr *b);
