#ifndef KEYS_H_
#define KEYS_H_

typedef struct melody_msg {
    double freq;
    int sg_num;
} melody_msg;

typedef struct melody_event {
    int         tick;
    melody_msg  mm;
} melody_event;

typedef struct melody_loop {
    melody_event melody[10];
    int size;
} melody_loop;


void keys(int soundgen_num);
void play_note(double freq, int sg_num, int recording); // recording is a boolean

melody_event make_melody_event(double freq, int sg_num, int tick);
void add_melody_event(melody_event);
void *play_melody_loop(void *m);

#endif // KEYS_H_
