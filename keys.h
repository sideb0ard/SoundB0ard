#ifndef KEYS_H_
#define KEYS_H_

typedef struct melody_event {
    int tick;
    double freq;
    char note[4];
} melody_event;

typedef struct melody_loop {
    melody_event *melody[100];

    int size;
    int sig_num;

} melody_loop;

void keys(int soundgen_num);

melody_loop *new_melody_loop(int sig_num);
melody_event *make_melody_event(int tick, double freq, char note[4]);

void add_melody_event(melody_loop *, melody_event *);

//void play_note(int sg_num, double freq, int drone);
void play_note(int sg_num, double freq);
void *play_melody_loop(void *m);

void keys_start_melody_player(int sig_num, char *pattern);

#endif // KEYS_H_
